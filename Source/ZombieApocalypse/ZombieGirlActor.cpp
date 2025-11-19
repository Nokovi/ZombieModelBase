// Copyright University of Inland Norway

#include "ZombieGirlActor.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimBlueprint.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AZombieGirlActor::AZombieGirlActor() {

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

	// Biting Behavior Defaults
	BiteRange = 100.0f;
	BiteCooldown = 2.0f;
	bEnableBiting = true;
	BiteSearchRadius = 300.0f;

	// Movement boundary defaults
	BoundaryBuffer = 200.0f;
	bUseCustomBoundaries = false;
	CustomBoundaryMin = FVector(-5000.0f, -5000.0f, -1000.0f);
	CustomBoundaryMax = FVector(5000.0f, 5000.0f, 1000.0f);

	// Initialize timers
	WanderTimer = 0.0f;
	WanderDirection = FMath::RandRange(0.0f, 360.0f);
	LastBiteTime = 0.0f;
	CurrentTarget = nullptr;
	DirectionChangeTimer = 0.0f;
	bTurningAroundFromBoundary = false;
	BoundaryTurnTimer = 0.0f;
}

// Called when the game starts or when spawned
void AZombieGirlActor::BeginPlay() {

	Super::BeginPlay();

	// Store initial location for wandering and as last valid position
	InitialLocation = GetActorLocation();
	LastValidPosition = InitialLocation;

	// Calculate world boundaries
	CalculateWorldBoundaries();

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

		// Handle biting behavior
		if (bEnableBiting) {

			HandleBitingBehavior(DeltaTime);
		}

		// Handle movement behavior
		if (bShouldWander) {
			if (CurrentTarget && IsValid(CurrentTarget)) {
				HandleTargetedMovement(DeltaTime);
			}
			else {
				HandleWanderingMovement(DeltaTime);
			}
		}
	}

	// Draw debug boundaries if enabled
	if (bDrawDebugBoundaries) {
		DrawDebugBoundaries();
	}
}

void AZombieGirlActor::HandleWanderingMovement(float DeltaTime) {

	if (!bHasValidBoundaries) return;

	FVector CurrentLocation = GetActorLocation();
	
	// Update direction change timer
	WanderTimer += DeltaTime;
	DirectionChangeTimer += DeltaTime;

	// Handle boundary turning
	if (bTurningAroundFromBoundary) {
		BoundaryTurnTimer += DeltaTime;
		if (BoundaryTurnTimer >= 1.0f) { // Turn for 1 second
			bTurningAroundFromBoundary = false;
			BoundaryTurnTimer = 0.0f;
		}
	}

	// Check if we need to turn due to boundaries
	bool bNearBoundary = IsNearBoundary(CurrentLocation, BoundaryBuffer);
	if (bNearBoundary && !bTurningAroundFromBoundary) {
		// Get direction away from boundary
		FVector CurrentDirection = FVector(
			FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
			FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
			0.0f
		);
		
		FVector BoundaryAvoidance = GetBoundaryAvoidanceDirection(CurrentLocation, CurrentDirection);
		WanderDirection = BoundaryAvoidance.Rotation().Yaw;
		bTurningAroundFromBoundary = true;
		BoundaryTurnTimer = 0.0f;
		WanderTimer = 0.0f; // Reset wander timer to prevent immediate direction change
	}
	// Normal direction changes (only if not handling boundary)
	else if (!bTurningAroundFromBoundary && WanderTimer >= FMath::RandRange(2.0f, 5.0f)) {
		WanderDirection = FMath::RandRange(0.0f, 360.0f);
		WanderTimer = 0.0f;
	}

	// Calculate movement direction
	FVector DirectionVector = FVector(
		FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
		FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
		0.0f
	);

	// Check if we're getting too far from initial position (wander radius check)
	float DistanceFromStart = FVector::Dist2D(CurrentLocation, InitialLocation);
	if (DistanceFromStart > WanderRadius) {
		// Turn back toward initial position
		DirectionVector = (InitialLocation - CurrentLocation).GetSafeNormal();
	}

	// Calculate new position
	FVector NewLocation = CurrentLocation + (DirectionVector * MovementSpeed * DeltaTime);
	
	// Clamp to boundaries
	FVector ClampedLocation = ClampToBoundaries(NewLocation);
	
	// Only move if the new location is valid and within boundaries
	if (IsWithinBoundaries(ClampedLocation)) {
		SetActorLocation(ClampedLocation);
		LastValidPosition = ClampedLocation;

		// Rotate to face movement direction
		FRotator NewRotation = DirectionVector.Rotation();
		SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	}
	else {
		// If we can't move, try to return to last valid position
		SetActorLocation(LastValidPosition);
	}
}

void AZombieGirlActor::HandleTargetedMovement(float DeltaTime) {

	if (!CurrentTarget || !bHasValidBoundaries) return;

	FVector TargetLocation = CurrentTarget->GetActorLocation();
	FVector MyLocation = GetActorLocation();
	float DistanceToTarget = FVector::Dist2D(MyLocation, TargetLocation);

	// Move toward target if not in bite range
	if (DistanceToTarget > BiteRange) {

		FVector DirectionToTarget = (TargetLocation - MyLocation).GetSafeNormal();
		FVector NewLocation = MyLocation + (DirectionToTarget * MovementSpeed * DeltaTime);
		
		// Clamp to boundaries
		FVector ClampedLocation = ClampToBoundaries(NewLocation);
		
		// Check if target is within boundaries - if not, abandon target
		if (!IsWithinBoundaries(TargetLocation)) {
			CurrentTarget = nullptr;
			return;
		}

		// Only move if the new location is within boundaries
		if (IsWithinBoundaries(ClampedLocation)) {
			SetActorLocation(ClampedLocation);
			LastValidPosition = ClampedLocation;

			// Face the target
			FRotator NewRotation = DirectionToTarget.Rotation();
			SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
		}
		else {
			// If we can't reach target due to boundaries, abandon it
			CurrentTarget = nullptr;
		}
	}
	else {
		// We're in range, attempt to bite
		TryToBiteNearbyTargets();
	}
}

void AZombieGirlActor::CalculateWorldBoundaries() {

	if (bUseCustomBoundaries) {
		WorldBoundaryMin = CustomBoundaryMin;
		WorldBoundaryMax = CustomBoundaryMax;
		bHasValidBoundaries = true;
		return;
	}

	// Try to automatically detect world boundaries
	UWorld* World = GetWorld();
	if (!World) {
		bHasValidBoundaries = false;
		return;
	}

	// Start with a default large boundary
	WorldBoundaryMin = FVector(-10000.0f, -10000.0f, -2000.0f);
	WorldBoundaryMax = FVector(10000.0f, 10000.0f, 2000.0f);

	// Try to find static mesh actors that could represent level boundaries
	bool bFoundAnyGeometry = false;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator) {
		AActor* Actor = *ActorIterator;
		if (!Actor || Actor == this) continue;

		// Look for actors with static mesh components (potential level geometry)
		UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>();
		if (MeshComp && MeshComp->GetStaticMesh()) {
			FVector ActorLocation = Actor->GetActorLocation();
			FVector ActorBounds = Actor->GetComponentsBoundingBox().GetSize();

			if (!bFoundAnyGeometry) {
				// First piece of geometry found, initialize boundaries
				WorldBoundaryMin = ActorLocation - ActorBounds * 0.5f;
				WorldBoundaryMax = ActorLocation + ActorBounds * 0.5f;
				bFoundAnyGeometry = true;
			}
			else {
				// Expand boundaries to include this geometry
				FVector GeomMin = ActorLocation - ActorBounds * 0.5f;
				FVector GeomMax = ActorLocation + ActorBounds * 0.5f;
				
				WorldBoundaryMin.X = FMath::Min(WorldBoundaryMin.X, GeomMin.X);
				WorldBoundaryMin.Y = FMath::Min(WorldBoundaryMin.Y, GeomMin.Y);
				WorldBoundaryMin.Z = FMath::Min(WorldBoundaryMin.Z, GeomMin.Z);
				
				WorldBoundaryMax.X = FMath::Max(WorldBoundaryMax.X, GeomMax.X);
				WorldBoundaryMax.Y = FMath::Max(WorldBoundaryMax.Y, GeomMax.Y);
				WorldBoundaryMax.Z = FMath::Max(WorldBoundaryMax.Z, GeomMax.Z);
			}
		}
	}

	// If no geometry found, use a reasonable default around spawn point
	if (!bFoundAnyGeometry) {
		FVector SpawnCenter = GetActorLocation();
		float DefaultSize = 5000.0f;
		WorldBoundaryMin = SpawnCenter - FVector(DefaultSize, DefaultSize, 1000.0f);
		WorldBoundaryMax = SpawnCenter + FVector(DefaultSize, DefaultSize, 1000.0f);
	}

	bHasValidBoundaries = true;

	UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: Calculated boundaries - Min: %s, Max: %s"), 
		*WorldBoundaryMin.ToString(), *WorldBoundaryMax.ToString());
}

FVector AZombieGirlActor::GetBoundaryAvoidanceDirection(const FVector& CurrentLocation, const FVector& CurrentDirection) {

	FVector AvoidanceDirection = FVector::ZeroVector;

	// Check each boundary and add avoidance force
	if (CurrentLocation.X <= WorldBoundaryMin.X + BoundaryBuffer) {
		AvoidanceDirection.X += 1.0f; // Move away from left boundary
	}
	if (CurrentLocation.X >= WorldBoundaryMax.X - BoundaryBuffer) {
		AvoidanceDirection.X -= 1.0f; // Move away from right boundary
	}
	if (CurrentLocation.Y <= WorldBoundaryMin.Y + BoundaryBuffer) {
		AvoidanceDirection.Y += 1.0f; // Move away from front boundary
	}
	if (CurrentLocation.Y >= WorldBoundaryMax.Y - BoundaryBuffer) {
		AvoidanceDirection.Y -= 1.0f; // Move away from back boundary
	}

	// If no specific avoidance needed, turn around
	if (AvoidanceDirection.IsNearlyZero()) {
		AvoidanceDirection = -CurrentDirection;
	}

	return AvoidanceDirection.GetSafeNormal();
}

bool AZombieGirlActor::IsNearBoundary(const FVector& Location, float Buffer) const {

	if (!bHasValidBoundaries) return false;

	return (Location.X <= WorldBoundaryMin.X + Buffer ||
			Location.X >= WorldBoundaryMax.X - Buffer ||
			Location.Y <= WorldBoundaryMin.Y + Buffer ||
			Location.Y >= WorldBoundaryMax.Y - Buffer);
}

void AZombieGirlActor::DrawDebugBoundaries() const {

	if (!bHasValidBoundaries || !GetWorld()) return;

	// Draw boundary box
	DrawDebugBox(GetWorld(), (WorldBoundaryMin + WorldBoundaryMax) * 0.5f, 
		(WorldBoundaryMax - WorldBoundaryMin) * 0.5f, FColor::Red, false, -1.0f, 0, 10.0f);

	// Draw boundary buffer zone
	FVector BufferMin = WorldBoundaryMin + FVector(BoundaryBuffer, BoundaryBuffer, 0);
	FVector BufferMax = WorldBoundaryMax - FVector(BoundaryBuffer, BoundaryBuffer, 0);
	DrawDebugBox(GetWorld(), (BufferMin + BufferMax) * 0.5f, 
		(BufferMax - BufferMin) * 0.5f, FColor::Yellow, false, -1.0f, 0, 5.0f);
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

void AZombieGirlActor::HandleBitingBehavior(float DeltaTime) {

	if (!SimulationController)
		return;

	LastBiteTime += DeltaTime;

	if (LastBiteTime < BiteCooldown)
		return;

	// Find a target if we don't have one
	if (!CurrentTarget || !IsValid(CurrentTarget) || !CurrentTarget->IsValidBiteTarget()) {

		CurrentTarget = FindNearestBiteTarget();
	}

	// Target handling is now done in HandleTargetedMovement
}

void AZombieGirlActor::TryToBiteNearbyTargets() {

	if (!SimulationController)
		return;

	// Check cooldown
	if (LastBiteTime < BiteCooldown)
		return;

	// Find all potential targets in bite range
	UWorld* World = GetWorld();
	if (!World)
		return;

	FVector MyLocation = GetActorLocation();

	for (TActorIterator<APopulationMeshActor> ActorIterator(World); ActorIterator; ++ActorIterator) {

		APopulationMeshActor* PotentialTarget = *ActorIterator;

		if (!PotentialTarget || PotentialTarget == this)
			continue;

		if (!CanBiteTarget(PotentialTarget))
			continue;

		// Check if target is within boundaries
		if (!IsWithinBoundaries(PotentialTarget->GetActorLocation()))
			continue;

		float Distance = FVector::Dist2D(MyLocation, PotentialTarget->GetActorLocation());

		if (Distance <= BiteRange) {

			// Get current simulation time
			float CurrentSimulationTime = static_cast<float>(SimulationController->TimeStepsFinished);

			// Bite the target
			PotentialTarget->GetBitten(CurrentSimulationTime);

			// Reset bite timer
			LastBiteTime = 0.0f;

			// Clear current target so we can find a new one
			CurrentTarget = nullptr;

			UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: %s bit %s at simulation time %f"),
				*GetName(), *PotentialTarget->GetName(), CurrentSimulationTime);

			// Only bite one target per attempt
			break;
		}
	}
}

APopulationMeshActor* AZombieGirlActor::FindNearestBiteTarget() {

	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	FVector MyLocation = GetActorLocation();
	APopulationMeshActor* NearestTarget = nullptr;
	float NearestDistance = BiteSearchRadius;

	for (TActorIterator<APopulationMeshActor> ActorIterator(World); ActorIterator; ++ActorIterator){

		APopulationMeshActor* PotentialTarget = *ActorIterator;

		if (!PotentialTarget || PotentialTarget == this)
			continue;

		if (!CanBiteTarget(PotentialTarget))
			continue;

		// Only consider targets within boundaries
		if (!IsWithinBoundaries(PotentialTarget->GetActorLocation()))
			continue;

		float Distance = FVector::Dist2D(MyLocation, PotentialTarget->GetActorLocation());

		if (Distance < NearestDistance) {

			NearestDistance = Distance;
			NearestTarget = PotentialTarget;
		}
	}

	return NearestTarget;
}

bool AZombieGirlActor::CanBiteTarget(APopulationMeshActor* Target) const {

	if (!Target)
		return false;

	return Target->IsValidBiteTarget();
}


