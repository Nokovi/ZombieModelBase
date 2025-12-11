// Copyright University of Inland Norway


#include "ZombieGirlSpawner.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// Sets default values
AZombieGirlSpawner::AZombieGirlSpawner() {

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Set Default Zombie Actor Class
	ZombieGirlActorClass = AZombieGirlActor::StaticClass();
}

// Called when the game starts or when spawned
void AZombieGirlSpawner::BeginPlay() {

	Super::BeginPlay();

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("ZombieGirlSpawner: BeginPlay called"));
	}

	if (bAutoSpawnOnBeginPlay) {

		if (bEnableDebugLogging) {

			UE_LOG(LogTemp, Warning, TEXT("ZombieGirlSpawner: Auto-spawning enabled, calling SpawnZombies function"));
		}

		SpawnZombies();
	}
}

// Called every frame
void AZombieGirlSpawner::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

}

void AZombieGirlSpawner::SpawnZombies() {

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("ZombieGirlSpawner: SpawnZombies() called"));
	}

	// Clear Existing Zombies First
	ClearSpawnedZombies();

	if (!ZombieGirlActorClass) {

		UE_LOG(LogTemp, Error, TEXT("ZombieGirlSpawner: No ZombieActorClass set!"));
		return;
	}

	if (!ZombieMesh) {

		UE_LOG(LogTemp, Error, TEXT("ZombieGirlSpawner: No ZombieMesh assigned! Please assign a skeletal mesh in the Zombie Assets section."));
		return;
	}

	UWorld* World = GetWorld();

	if (!World) {

		UE_LOG(LogTemp, Error, TEXT("ZombieGirlSpawner: No valid world found!"));
		return;
	}

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Warning, TEXT("ZombieGirlSpawner: Starting to spawn %d zombies"), NumberToSpawn);
	}

	// Spawn the specified number of zombies
	int32 SuccessfulSpawns = 0;
	for (int32 i = 0; i < NumberToSpawn; i++) {

		FVector SpawnLocation = CalculateSpawnLocation(i);
		FRotator SpawnRotation = GetActorRotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AZombieGirlActor* SpawnedZombie = World->SpawnActor<AZombieGirlActor>(

			ZombieGirlActorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (SpawnedZombie) {

			// Configure the spawned zombie
			ConfigureSpawnedZombie(SpawnedZombie);

			// Add to tracking Array
			SpawnedZombies.Add(SpawnedZombie);
			SuccessfulSpawns++;

			if (bEnableDebugLogging && (i < 5 || i % 20 == 0)) {

				UE_LOG(LogTemp, Log, TEXT("ZombieGirlSpawner: Successfully spawned zombie %d at location %s"), i, *SpawnLocation.ToString());
			}
		}

		else {

			UE_LOG(LogTemp, Error, TEXT("ZombieGirlSpawner: Failed to spawn zombie at index %d"), i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ZombieGirlSpawner: Successfully spawned %d/%d zombies"), SuccessfulSpawns, NumberToSpawn);
}

void AZombieGirlSpawner::ConfigureSpawnedZombie(AZombieGirlActor* SpawnedZombie) {

	if (!SpawnedZombie) return;

	// Set population type to Zombie
	SpawnedZombie->PopulationType = EPopulationType::Zombie;

	// Assign the mesh assets
	SpawnedZombie->ZombieMesh = ZombieMesh;
	if (ZombieAnimBP) {

		SpawnedZombie->ZombieAnimBP = ZombieAnimBP;
	}

	// Make it use skeletal mesh
	SpawnedZombie->bUseSkeletalMesh = true;

	// Disable automatic simulation controller finding to prevent warnings
	SpawnedZombie->bAutoFindSimulationController = false;

	// Mesh setup
	if (SpawnedZombie->SkeletalMeshComponent && ZombieMesh) {

		SpawnedZombie->SkeletalMeshComponent->SetSkeletalMesh(ZombieMesh);
		SpawnedZombie->SkeletalMeshComponent->SetVisibility(true);

		if (ZombieAnimBP && ZombieAnimBP->GetAnimBlueprintGeneratedClass()) {

			SpawnedZombie->SkeletalMeshComponent->SetAnimInstanceClass(ZombieAnimBP->GetAnimBlueprintGeneratedClass());
		}
	}

	// Hide static mesh component
	if (SpawnedZombie->StaticMeshComponent) {

		SpawnedZombie->StaticMeshComponent->SetVisibility(false);
	}

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Log, TEXT("ZombieGirlSpawner: Configured zombie with mesh: %s"), ZombieMesh ? *ZombieMesh->GetName() : TEXT("None"));
	}
}

void AZombieGirlSpawner::ClearSpawnedZombies() {

	CleanupSpawnedActors();
}

void AZombieGirlSpawner::CleanupSpawnedActors() {

	// Destroy all previously spawned zombies
	for (AZombieGirlActor* Zombie : SpawnedZombies) {

		if (IsValid(Zombie)) {

			Zombie->Destroy();
		}
	}

	SpawnedZombies.Empty();

	if (bEnableDebugLogging) {

		UE_LOG(LogTemp, Log, TEXT("ZombieGirlSpawner: Cleaned up all spawned zombies"));
	}
}

FVector AZombieGirlSpawner::CalculateSpawnLocation(int32 Index) const {

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

