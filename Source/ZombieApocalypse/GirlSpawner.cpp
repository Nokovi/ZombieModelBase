#include "GirlSpawner.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// Sets default values
AGirlSpawner::AGirlSpawner() {

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Set Default Girl Actor Class
	GirlActorClass = APopulationMeshActor::StaticClass();

}

// Called when the game starts or when spawned
void AGirlSpawner::BeginPlay() {

	Super::BeginPlay();

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("GirlSpawner: BeginPlay called"));
	}

	if (bAutoSpawnOnBeginPlay) {

		if (bEnableDebugLogging) {

			UE_LOG(LogTemp, Warning, TEXT("GirlSpawner: Auto-spawning enabled, calling SpawnGirls function"));
		}

		SpawnGirls();
	}
}

// Called every frame
void AGirlSpawner::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

}

void AGirlSpawner::SpawnGirls() {

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("GirlSpawner: SpawnGirls() called"));
	}

	// Clear Existing Girls First
	ClearSpawnedGirls();

	if (!GirlActorClass) {

		UE_LOG(LogTemp, Error, TEXT("GirlSpawner: No GirlActorClass set!"));
		return;
	}

	if (!GirlMesh) {

		UE_LOG(LogTemp, Error, TEXT("GirlSpawner: No GirlMesh assigned! Please assign a skeletal mesh in the Girl Assets section."));
		return;
	}

	UWorld* World = GetWorld();

	if (!World) {

		UE_LOG(LogTemp, Error, TEXT("GirlSpawner: No valid world found!"));
		return;
	}

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("GirlSpawner: Starting to spawn %d girls"), NumberToSpawn);
	}

	// Spawn the specified number of girls
	int32 SuccessfulSpawns = 0;
	for (int32 i = 0; i < NumberToSpawn; i++) {

		FVector SpawnLocation = CalculateSpawnLocation(i);
		FRotator SpawnRotation = GetActorRotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APopulationMeshActor* SpawnedGirl = World->SpawnActor<APopulationMeshActor>(
			GirlActorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (SpawnedGirl) {
			// Configure the spawned girl
			ConfigureSpawnedGirl(SpawnedGirl);

			// Add to tracking Array
			SpawnedGirls.Add(SpawnedGirl);
			SuccessfulSpawns++;

			if (bEnableDebugLogging && (i < 5 || i % 20 == 0)) {

				UE_LOG(LogTemp, Log, TEXT("GirlSpawner: Successfully spawned girl %d at location %s"), i, *SpawnLocation.ToString());
			}
		}

		else {

			UE_LOG(LogTemp, Error, TEXT("GirlSpawner: Failed to spawn girl at index %d"), i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("GirlSpawner: Successfully spawned %d/%d girls"), SuccessfulSpawns, NumberToSpawn);
}

void AGirlSpawner::ConfigureSpawnedGirl(APopulationMeshActor* SpawnedGirl) {

	if (!SpawnedGirl) return;

	// Set population type to Susceptible for girls
	SpawnedGirl->PopulationType = EPopulationType::Susceptible;

	// Assign the mesh assets
	SpawnedGirl->SusceptibleMesh = GirlMesh;
	if (GirlAnimBP) {

		SpawnedGirl->SusceptibleAnimBP = GirlAnimBP;
	}

	// Make it use skeletal mesh
	SpawnedGirl->bUseSkeletalMesh = true;

	// Disable automatic simulation controller finding to prevent warnings
	SpawnedGirl->bAutoFindSimulationController = false;

	// Mesh setup
	if (SpawnedGirl->SkeletalMeshComponent && GirlMesh) {

		SpawnedGirl->SkeletalMeshComponent->SetSkeletalMesh(GirlMesh);
		SpawnedGirl->SkeletalMeshComponent->SetVisibility(true);

		if (GirlAnimBP && GirlAnimBP->GetAnimBlueprintGeneratedClass()) {

			SpawnedGirl->SkeletalMeshComponent->SetAnimInstanceClass(GirlAnimBP->GetAnimBlueprintGeneratedClass());
		}
	}

	// Hide static mesh component
	if (SpawnedGirl->StaticMeshComponent) {

		SpawnedGirl->StaticMeshComponent->SetVisibility(false);
	}

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Log, TEXT("GirlSpawner: Configured girl with mesh: %s"), GirlMesh ? *GirlMesh->GetName() : TEXT("None"));
	}
}

void AGirlSpawner::ClearSpawnedGirls() {

	CleanupSpawnedActors();
}

void AGirlSpawner::CleanupSpawnedActors() {

	// Destroy all previously spawned girls
	for (APopulationMeshActor* Girl : SpawnedGirls) {

		if (IsValid(Girl)) {

			Girl->Destroy();
		}
	}

	SpawnedGirls.Empty();

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Log, TEXT("GirlSpawner: Cleaned up all spawned girls"));
	}
}

FVector AGirlSpawner::CalculateSpawnLocation(int32 Index) const {

	// Calculate grid position
	int32 Row = Index / ActorsPerRow;
	int32 Column = Index % ActorsPerRow;

	// Calculate offset from spawner location
	float XOffset = Column * SpacingBetweenActors;
	float YOffset = Row * SpacingBetweenActors;

	// Apply Spawner's transform
	FVector LocalOffset(XOffset, YOffset, 0.0f);
	FVector WorldOffset = GetActorTransform().TransformVector(LocalOffset);

	return GetActorLocation() + WorldOffset;
}

