// Copyright University of Inland Norway

#include "ZombieGirlActor.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimBlueprint.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AZombieGirlActor::AZombieGirlActor() {

	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Set default population type to Bitten (closest to zombie representation)
	PopulationType = EPopulationType::Bitten;

	// Visual defaults
	bEnableZombieEffects = true;
	ZombieTint = FLinearColor(0.8f, 1.0f, 0.8f, 1.0f);

	// NEW: Teleportation defaults (replacing movement)
	TeleportInterval = 1.0f; // Teleport every second
	TeleportRange = 150.0f;
	bEnableTeleportation = true;
	bEnableDebugTeleport = true;

	// Biting Behavior Defaults
	BiteRange = 100.0f;
	BiteCooldown = 0.5f; // Faster biting since we teleport
	bEnableBiting = true;

	// Disable movement - zombies only teleport now
	bShouldWander = false;
	MovementSpeed = 0.0f;

	// Initialize timers
	TeleportTimer = 0.0f;
	LastBiteTime = 0.0f;
}

// Called when the game starts or when spawned
void AZombieGirlActor::BeginPlay() {

	Super::BeginPlay();

	// Setup zombie-specific components
	SetupZombieComponents();
}

// Called every frame
void AZombieGirlActor::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

	// Handle zombie-specific updates
	if (SimulationController) {

		float CurrentZombieCount = GetZombiePopulation();

		// Update zombie representation when population changes
		if (CurrentZombieCount != PreviousZombieCount) {

			UpdateZombieMesh();
			PreviousZombieCount = CurrentZombieCount;
		}

		// NEW: Handle teleportation behavior (replaces movement)
		if (bEnableTeleportation) {
			HandleZombieTeleportation(DeltaTime);
		}
	}
}

void AZombieGirlActor::HandleZombieTeleportation(float DeltaTime) {

	if (!SimulationController)
		return;

	// Update teleport timer
	TeleportTimer += DeltaTime;
	LastBiteTime += DeltaTime;

	// Check if it's time to teleport (every second)
	if (TeleportTimer >= TeleportInterval) {

		// Find a random girl to teleport to
		APopulationMeshActor* RandomTarget = FindRandomBiteTarget();
		
		if (RandomTarget) {
			
			// Teleport to the target
			TeleportToTarget(RandomTarget);
			
			// Attempt to bite after teleporting
			AttemptBiteAfterTeleport(RandomTarget);
			
			if (bEnableDebugTeleport) {
				UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: %s teleported to bite target %s"), 
					*GetName(), *RandomTarget->GetName());
			}
		}
		else {
			if (bEnableDebugTeleport) {
				UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: %s could not find any bite targets"), *GetName());
			}
		}

		// Reset teleport timer
		TeleportTimer = 0.0f;
	}
}

APopulationMeshActor* AZombieGirlActor::FindRandomBiteTarget() {

	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	// Collect all valid bite targets
	TArray<APopulationMeshActor*> ValidTargets;

	for (TActorIterator<APopulationMeshActor> ActorIterator(World); ActorIterator; ++ActorIterator) {

		APopulationMeshActor* PotentialTarget = *ActorIterator;

		if (!PotentialTarget || PotentialTarget == this)
			continue;

		// Check if this is a valid bite target (susceptible girls)
		if (PotentialTarget->IsValidBiteTarget()) {
			ValidTargets.Add(PotentialTarget);
		}
	}

	// Return a random target from valid targets
	if (ValidTargets.Num() > 0) {
		int32 RandomIndex = FMath::RandRange(0, ValidTargets.Num() - 1);
		return ValidTargets[RandomIndex];
	}

	return nullptr;
}

void AZombieGirlActor::TeleportToTarget(APopulationMeshActor* Target) {

	if (!Target)
		return;

	// Get target's location
	FVector TargetLocation = Target->GetActorLocation();
	
	// Calculate a random position near the target (within teleport range)
	float RandomAngle = FMath::RandRange(0.0f, 360.0f);
	float RandomDistance = FMath::RandRange(BiteRange * 0.5f, TeleportRange);
	
	FVector OffsetDirection = FVector(
		FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
		0.0f
	);
	
	FVector TeleportLocation = TargetLocation + (OffsetDirection * RandomDistance);
	
	// Teleport to the calculated position
	SetActorLocation(TeleportLocation);
	
	// Face the target
	FVector DirectionToTarget = (TargetLocation - TeleportLocation).GetSafeNormal();
	FRotator NewRotation = DirectionToTarget.Rotation();
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void AZombieGirlActor::AttemptBiteAfterTeleport(APopulationMeshActor* Target) {

	if (!Target || !SimulationController)
		return;

	// Check bite cooldown
	if (LastBiteTime < BiteCooldown)
		return;

	// Check if target is still valid and in range after teleportation
	if (!Target->IsValidBiteTarget())
		return;

	float DistanceToTarget = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
	
	if (DistanceToTarget <= BiteRange) {

		// Get current simulation time
		float CurrentSimulationTime = static_cast<float>(SimulationController->TimeStepsFinished);

		// Bite the target
		Target->GetBitten(CurrentSimulationTime);

		// Reset bite timer
		LastBiteTime = 0.0f;

		if (bEnableDebugTeleport) {
			UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: %s successfully bit %s after teleportation at simulation time %f"),
				*GetName(), *Target->GetName(), CurrentSimulationTime);
		}
	}
}

void AZombieGirlActor::SetupZombieComponents() {

	// Configure zombie mesh
	UpdateZombieMesh();

	// Apply zombie visual effects
	if (bEnableZombieEffects && SkeletalMeshComponent) {

		// Create dynamic material instance for zombie tint
		if (UMaterialInterface* CurrentMaterial = SkeletalMeshComponent->GetMaterial(0)) {

			UMaterialInstanceDynamic* DynamicMaterial = SkeletalMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
			if (DynamicMaterial) {

				DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), ZombieTint);
			}
		}
	}
}

void AZombieGirlActor::UpdateZombieMesh() {

	if (!SkeletalMeshComponent)
		return;

	// Use zombie-specific mesh if available, otherwise fall back to bitten mesh
	USkeletalMesh* MeshToUse = ZombieMesh ? ZombieMesh : BittenMesh;
	UAnimBlueprint* AnimToUse = ZombieAnimBP ? ZombieAnimBP : BittenAnimBP;

	if (MeshToUse) {

		SkeletalMeshComponent->SetSkeletalMesh(MeshToUse);
		SkeletalMeshComponent->SetVisibility(true);

		// Apply zombie animation
		if (AnimToUse && AnimToUse->GetAnimBlueprintGeneratedClass()) {

			SkeletalMeshComponent->SetAnimInstanceClass(AnimToUse->GetAnimBlueprintGeneratedClass());
		}
	}

	// Hide static mesh component since we're using skeletal mesh
	if (StaticMeshComponent) {

		StaticMeshComponent->SetVisibility(false);
	}
}

float AZombieGirlActor::GetZombiePopulation() const {

	if (!SimulationController)
		return 0.0f;

	return SimulationController->Zombies;
}