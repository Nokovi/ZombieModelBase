#include "BiteManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// Sets default values
ABiteManager::ABiteManager() {

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TransformationDays = 15.0f;
	bEnableDebugLogging = true;
	NextAvailableTransformationDay = 15; // Start transformations at day 15
}

// Called when the game starts or when spawned
void ABiteManager::BeginPlay() {

	Super::BeginPlay();

	if (!SimulationController) {

		FindSimulationController();
	}
}

// Called every frame
void ABiteManager::Tick(float DeltaTime){

	Super::Tick(DeltaTime);

	// Clean up invalid bites
	CleanupInvalidBites();

	// Schedule transformation days for any unscheduled bites
	SchedulePendingTransformations();

	// Check for transformations
	CheckForTransformations();
}

void ABiteManager::RegisterBite(APopulationMeshActor* BittenActor, float BiteTime) {

	if (!BittenActor)
		return;

	// Check if this actor is already registered
	for (const FBiteRecord& Record : ActiveBites) {

		if (Record.BittenActor == BittenActor) {

			if (bEnableDebugLogging) {

				UE_LOG(LogTemp, Warning, TEXT("BiteManager: Actor %s is already registered as bitten"), *BittenActor->GetName());
			}

			return;
		}
	}

	// Register new bite (transformation day will be scheduled in SchedulePendingTransformations)
	ActiveBites.Add(FBiteRecord(BittenActor, BiteTime));

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("BiteManager: Registered bite for %s at time %f"), *BittenActor->GetName(), BiteTime);
	}
}

void ABiteManager::SchedulePendingTransformations() {
    if (!SimulationController)
        return;

    int32 CurrentDay = SimulationController->TimeStepsFinished;
    
    // Only start processing transformations after day 15 (when simulation allows it)
    if (CurrentDay < 15) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("BiteManager: Waiting for day 15 to start transformations (Current day: %d)"), CurrentDay);
        }
        return;
    }

    // Schedule transformation days for any bites that don't have one assigned yet
    for (FBiteRecord& Record : ActiveBites) {
        if (Record.ScheduledTransformationDay == -1) {
            
            float TimeSinceBite = CurrentDay - Record.BiteTime;
            
            // Only schedule if enough time has passed since the bite AND we're past day 15
            if (TimeSinceBite >= TransformationDays) {
                
                // Ensure we don't schedule before day 15
                int32 EarliestTransformationDay = FMath::Max(NextAvailableTransformationDay, 15);
                
                Record.ScheduledTransformationDay = EarliestTransformationDay;
                NextAvailableTransformationDay = FMath::Max(NextAvailableTransformationDay + 1, 16); // Next bite gets scheduled for the following day

                if (bEnableDebugLogging) {
                    UE_LOG(LogTemp, Warning, TEXT("BiteManager: Scheduled %s for transformation on day %d"), 
                        *Record.BittenActor->GetName(), Record.ScheduledTransformationDay);
                }
            }
        }
    }
}

void ABiteManager::CheckForTransformations() {
    if (!SimulationController)
        return;

    int32 CurrentDay = SimulationController->TimeStepsFinished;
    
    // Don't process any transformations before day 15
    if (CurrentDay < 15) {
        return;
    }

    if (bSyncWithSimulationController) {
        // Calculate how many actors should transform based on simulation controller's conveyor outflow
        float SimulationBitten = SimulationController->Bitten;
        float PreviousSimulationBitten = LastKnownSimulationBitten; // You'll need to track this
        
        // If simulation bitten population decreased, it means some transformed to zombies
        if (PreviousSimulationBitten > SimulationBitten) {
            float TransformedInSimulation = PreviousSimulationBitten - SimulationBitten;
            int32 ActorsToTransform = FMath::RoundToInt(TransformedInSimulation);
            
            // Transform the oldest scheduled actors
            TransformOldestScheduledActors(ActorsToTransform, CurrentDay);
        }
        
        LastKnownSimulationBitten = SimulationBitten;
    } else {
        // Use your original individual timing approach
        CheckForIndividualTransformations(CurrentDay);
    }
}

int32 ABiteManager::GetActiveBiteCount() const {

	return ActiveBites.Num();
}

void ABiteManager::FindSimulationController() {

	if (UWorld* World = GetWorld()) {

		for (TActorIterator<ASimulationController> ActorIterator(World); ActorIterator; ++ActorIterator) {

			SimulationController = *ActorIterator;
			break;
		}

		if (!SimulationController) {

			UE_LOG(LogTemp, Warning, TEXT("BiteManager: Could not find Simulation Controller"));
		}
	}
}

void ABiteManager::CleanupInvalidBites() {

	for (int32 i = ActiveBites.Num() - 1; i >= 0; --i) {

		if (!ActiveBites[i].BittenActor.IsValid()) {

			ActiveBites.RemoveAt(i);
		}
	}
}

void ABiteManager::TransformOldestScheduledActors(int32 ActorsToTransform, int32 CurrentDay)
{
    if (ActorsToTransform <= 0)
    {
        return;
    }

    // Find all actors scheduled for transformation on the current day
    TArray<int32> CandidateIndices;
    for (int32 i = 0; i < ActiveBites.Num(); i++)
    {
        if (ActiveBites[i].ScheduledTransformationDay == CurrentDay &&
            ActiveBites[i].BittenActor.IsValid())
        {
            CandidateIndices.Add(i);
        }
    }

    if (CandidateIndices.Num() == 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("BiteManager: No valid actors scheduled for transformation on day %d"), CurrentDay);
        }
        return;
    }

    // Sort candidates by bite time (oldest first)
    CandidateIndices.Sort([this](const int32& A, const int32& B)
        {
            return ActiveBites[A].BiteTime < ActiveBites[B].BiteTime;
        });

    // Transform the oldest scheduled actors up to the requested amount
    int32 ActualTransformCount = FMath::Min(ActorsToTransform, CandidateIndices.Num());
    TArray<int32> IndicesToRemove;

    for (int32 i = 0; i < ActualTransformCount; i++)
    {
        int32 BiteIndex = CandidateIndices[i];
        APopulationMeshActor* ActorToTransform = ActiveBites[BiteIndex].BittenActor.Get();

        if (ActorToTransform)
        {
            // Transform the actor to zombie
            ActorToTransform->TransformToZombie();

            if (bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Log, TEXT("BiteManager: Transformed actor to zombie on day %d (bite time: %.2f)"),
                    CurrentDay, ActiveBites[BiteIndex].BiteTime);
            }
        }

        // Mark this bite record for removal
        IndicesToRemove.Add(BiteIndex);
    }

    // Remove transformed bite records (remove in reverse order to maintain indices)
    IndicesToRemove.Sort([](const int32& A, const int32& B) { return A > B; });
    for (int32 IndexToRemove : IndicesToRemove)
    {
        ActiveBites.RemoveAt(IndexToRemove);
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("BiteManager: Transformed %d actors on day %d. %d active bites remaining"),
            ActualTransformCount, CurrentDay, ActiveBites.Num());
    }
}

void ABiteManager::CheckForIndividualTransformations(int32 CurrentDay)
{
    if (ActiveBites.Num() == 0)
    {
        return;
    }

    TArray<int32> IndicesToRemove;
    
    // Check each bite record for transformation eligibility
    for (int32 i = ActiveBites.Num() - 1; i >= 0; --i)
    {
        FBiteRecord& BiteRecord = ActiveBites[i];
        
        // Check if actor is still valid
        if (!BiteRecord.BittenActor.IsValid())
        {
            IndicesToRemove.Add(i);
            continue;
        }
        
        // Check if transformation day has arrived
        if (BiteRecord.ScheduledTransformationDay <= CurrentDay)
        {
            APopulationMeshActor* ActorToTransform = BiteRecord.BittenActor.Get();
            
            if (ActorToTransform)
            {
                // Transform the actor to zombie state
                ActorToTransform->TransformToZombie();  // Changed from SetActorState
                
                if (bEnableDebugLogging)
                {
                    UE_LOG(LogTemp, Log, TEXT("BiteManager: Individual transformation - Actor transformed to zombie on day %d (scheduled day: %d)"), 
                           CurrentDay, BiteRecord.ScheduledTransformationDay);
                }
            }
            
            // Mark for removal from active bites
            IndicesToRemove.Add(i);
        }
    }
    
    // Remove transformed or invalid bite records
    for (int32 Index : IndicesToRemove)
    {
        ActiveBites.RemoveAt(Index);
    }
    
    if (bEnableDebugLogging && IndicesToRemove.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("BiteManager: Processed %d individual transformations on day %d. Active bites remaining: %d"), 
               IndicesToRemove.Num(), CurrentDay, ActiveBites.Num());
    }
}