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
	TeleportInterval = 5.0f; // Teleport (Specify how many seconds -- Default 5 seconds)
	TeleportRange = 150.0f;
	bEnableTeleportation = true;
	bEnableDebugTeleport = true;

	// Biting Behavior Defaults
	BiteRange = 100.0f;
	BiteCooldown = 5.0f; // Faster biting since we teleport
	bEnableBiting = true;

	// Disable movement - zombies only teleport now
	bShouldWander = false;
	MovementSpeed = 0.0f;

	// Initialize timers
	TeleportTimer = 5.0f;
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

// Add these methods to inherit boundary functionality
bool AZombieGirlActor::ZombieIsWithinBoundaries(const FVector& Location) const {
	// Delegate to parent class boundary checking
	return Super::IsWithinBoundaries(Location);
}

FVector AZombieGirlActor::ZombieClampToBoundaries(const FVector& Location) const {
	// Delegate to parent class boundary clamping
	return Super::ClampToBoundaries(Location);
}