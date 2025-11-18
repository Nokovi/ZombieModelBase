#include "BiteManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// Sets default values
ABiteManager::ABiteManager() {

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TransformationDays = 15.0f;
	bEnableDebugLogging = true;
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

	// Register new bite
	ActiveBites.Add(FBiteRecord(BittenActor, BiteTime));

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("BiteManager: Registered bite for %s at time %f"), *BittenActor->GetName(), BiteTime);
	}
}

void ABiteManager::CheckForTransformations() {

	if (!SimulationController)
		return;

	float CurrentTime = static_cast<float>(SimulationController->TimeStepsFinished);

	for (int32 i = ActiveBites.Num() - 1; i >= 0; --i) {

		FBiteRecord& Record = ActiveBites[i];

		if (!Record.BittenActor.IsValid()) {

			ActiveBites.RemoveAt(i);
			continue;
		}

		APopulationMeshActor* BittenActor = Record.BittenActor.Get();
		float TimeSinceBite = CurrentTime - Record.BiteTime;

		if (TimeSinceBite >= TransformationDays) {

			// Transform to zombie
			BittenActor->TransformToZombie();

			// Remove from active bites
			ActiveBites.RemoveAt(i);

			if (bEnableDebugLogging) {

				UE_LOG(LogTemp, Warning, TEXT("BiteManager: %s transformed to zombie after %f days"),
					*BittenActor->GetName(), TimeSinceBite);
			}
		}
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