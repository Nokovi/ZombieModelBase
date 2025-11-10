// Copyright University of Inland Norway


#include "ZombieGirlActor.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimBlueprint.h"

// Sets default values
AZombieGirlActor::AZombieGirlActor()
{
	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Set default population type to Bitten (closest to zombie representation)
	PopulationType = EPopulationType::Bitten;

	// Zombie-specific defaults
	MovementSpeed = 250.0f;
	AttackRange = 75.0f;
	bShouldWander = true;
	WanderRadius = 2000.0f;
	bEnableZombieEffects = true;
	ZombieTint = FLinearColor(0.8f, 1.0f, 0.8f, 1.0f);

	// Initialize timers
	WanderTimer = 0.0f;
	WanderDirection = FMath::RandRange(0.0f, 360.0f);
}

// Called when the game starts or when spawned
void AZombieGirlActor::BeginPlay()
{
	Super::BeginPlay();

	// Store initial location for wandering
	InitialLocation = GetActorLocation();

	// Setup zombie-specific components
	SetupZombieComponents();
}

// Called every frame
void AZombieGirlActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle zombie-specific updates
	if (SimulationController)
	{
		float CurrentZombieCount = GetZombiePopulation();

		// Update zombie representation when population changes
		if (CurrentZombieCount != PreviousZombieCount)
		{
			UpdateZombieMesh();
			PreviousZombieCount = CurrentZombieCount;
		}

		// Handle wandering behavior
		if (bShouldWander)
		{
			WanderTimer += DeltaTime;

			// Change direction every 2-5 seconds
			if (WanderTimer >= FMath::RandRange(2.0f, 5.0f))
			{
				WanderDirection = FMath::RandRange(0.0f, 360.0f);
				WanderTimer = 0.0f;
			}

			// Move in the wander direction
			FVector CurrentLocation = GetActorLocation();
			FVector DirectionVector = FVector(
				FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
				FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
				0.0f
			);

			// Check if we're still within wander radius
			float DistanceFromStart = FVector::Dist2D(CurrentLocation, InitialLocation);
			if (DistanceFromStart > WanderRadius)
			{
				// Turn back toward initial position
				DirectionVector = (InitialLocation - CurrentLocation).GetSafeNormal();
			}

			// Apply movement
			FVector NewLocation = CurrentLocation + (DirectionVector * MovementSpeed * DeltaTime);
			SetActorLocation(NewLocation);

			// Rotate to face movement direction
			FRotator NewRotation = DirectionVector.Rotation();
			SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
		}
	}
}

void AZombieGirlActor::SetupZombieComponents()
{
	// Configure zombie mesh
	UpdateZombieMesh();

	// Apply zombie visual effects
	if (bEnableZombieEffects && SkeletalMeshComponent)
	{
		// Create dynamic material instance for zombie tint
		if (UMaterialInterface* CurrentMaterial = SkeletalMeshComponent->GetMaterial(0))
		{
			UMaterialInstanceDynamic* DynamicMaterial = SkeletalMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), ZombieTint);
			}
		}
	}
}

void AZombieGirlActor::UpdateZombieMesh()
{
	if (!SkeletalMeshComponent)
		return;

	// Use zombie-specific mesh if available, otherwise fall back to bitten mesh
	USkeletalMesh* MeshToUse = ZombieMesh ? ZombieMesh : BittenMesh;
	UAnimBlueprint* AnimToUse = ZombieAnimBP ? ZombieAnimBP : BittenAnimBP;

	if (MeshToUse)
	{
		SkeletalMeshComponent->SetSkeletalMesh(MeshToUse);
		SkeletalMeshComponent->SetVisibility(true);

		// Apply zombie animation
		if (AnimToUse && AnimToUse->GetAnimBlueprintGeneratedClass())
		{
			SkeletalMeshComponent->SetAnimInstanceClass(AnimToUse->GetAnimBlueprintGeneratedClass());
		}
	}

	// Hide static mesh component since we're using skeletal mesh
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetVisibility(false);
	}
}

float AZombieGirlActor::GetZombiePopulation() const
{
	if (!SimulationController)
		return 0.0f;

	return SimulationController->Zombies;
}

