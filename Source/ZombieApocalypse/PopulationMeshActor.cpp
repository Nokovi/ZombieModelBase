// Copyright University of Inland Norway


#include "PopulationMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "SimulationController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimBlueprint.h"

// Sets default values
APopulationMeshActor::APopulationMeshActor() {

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create the Root Component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create the Skeletal Mesh Component
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);

	// Create Static Mesh Component
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);

	// Skibidi Default Values
	PopulationType = EPopulationType::Susceptible;
	ScaleMultiplier = 1.0f;
	bUseSkeletalMesh = true;
	bAutoFindSimulationController = true;
	PreviousPopulationValue = 0.0f;
	PreviousPopulationType = EPopulationType::Susceptible;

	// Bite Management System Defaults
	bIsBitten = false;
	BittenTimestamp = -1.0f;
	bCanBeBitten = true;

	// Zombie Biting Behavior Defaults
	BiteRange = 100.0f;
	BiteCooldown = 5.0f;
	bEnableBiting = true;
	BiteSearchRadius = 300.0f;
	LastBiteTime = 0.0f;

	// Zombie Teleportation Defaults
	TeleportInterval = 5.0f;
	TeleportRange = 150.0f;
	bEnableTeleportation = true;
	bEnableDebugTeleport = false;
	TeleportTimer = 0.0f;

	// Hides Static Mesh if Skeletal Mesh is used
	StaticMeshComponent->SetVisibility(!bUseSkeletalMesh);
	SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh);

	MovementSpeed = 250.0f;
	AttackRange = 75.0f;
	bShouldWander = true;
	WanderRadius = 2000.0f;

	// Boundary restrictions to keep girls within level
	BoundaryBuffer = 200.0f; // Distance from edge before turning around
	bUseCustomBoundaries = true; // Custom boundaries

	// Set reasonable default boundaries (adjust these values for your level)
	CustomBoundaryMin = FVector(-2500.0f, -2500.0f, -500.0f);
	CustomBoundaryMax = FVector(2500.0f, 2500.0f, 500.0f);

	bDrawDebugBoundaries = true; // Enable debug visualization

	// Initialize timers
	WanderTimer = 0.0f;
	WanderDirection = FMath::RandRange(0.0f, 360.0f);
	DirectionChangeTimer = 0.0f;
	bTurningAroundFromBoundary = false;
	BoundaryTurnTimer = 0.0f;


	//Create weapon collision component
	WeaponCollider = CreateDefaultSubobject<UCapsuleComponent> (TEXT("Weapon Collider"));
	WeaponCollider->SetCapsuleSize(50.f, 180.f, true);
	WeaponCollider->SetupAttachment(RootComponent);
	WeaponCollider->SetGenerateOverlapEvents(true);

}

// Called when the game starts or when spawned
void APopulationMeshActor::BeginPlay() {

	Super::BeginPlay();

	// AutoFind Simulation Controller if Enabled
	if (bAutoFindSimulationController && !SimulationController) {

		FindSimulationController();
	}

	// Setup Mesh
	SetupMeshComponent();
	UpdateMeshBasedOnPopulation();

	// Store initial location for wandering and as last valid position
	InitialLocation = GetActorLocation();
	LastValidPosition = InitialLocation;

	// Calculate world boundaries
	CalculateWorldBoundaries();
}

void APopulationMeshActor::CalculateWorldBoundaries() {

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

	UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Calculated boundaries - Min: %s, Max: %s"),
		*WorldBoundaryMin.ToString(), *WorldBoundaryMax.ToString());
}

FVector APopulationMeshActor::GetBoundaryAvoidanceDirection(const FVector& CurrentLocation, const FVector& CurrentDirection) {

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

bool APopulationMeshActor::IsNearBoundary(const FVector& Location, float Buffer) const {

	if (!bHasValidBoundaries) return false;

	return (Location.X <= WorldBoundaryMin.X + Buffer ||
		Location.X >= WorldBoundaryMax.X - Buffer ||
		Location.Y <= WorldBoundaryMin.Y + Buffer ||
		Location.Y >= WorldBoundaryMax.Y - Buffer);
}

bool APopulationMeshActor::IsWithinBoundaries(const FVector& Location) const {

	if (!bHasValidBoundaries) return true;

	return (Location.X >= WorldBoundaryMin.X &&
		Location.X <= WorldBoundaryMax.X &&
		Location.Y >= WorldBoundaryMin.Y &&
		Location.Y <= WorldBoundaryMax.Y &&
		Location.Z >= WorldBoundaryMin.Z &&
		Location.Z <= WorldBoundaryMax.Z);
}

FVector APopulationMeshActor::ClampToBoundaries(const FVector& Location) const {

	if (!bHasValidBoundaries) return Location;

	return FVector(
		FMath::Clamp(Location.X, WorldBoundaryMin.X, WorldBoundaryMax.X),
		FMath::Clamp(Location.Y, WorldBoundaryMin.Y, WorldBoundaryMax.Y),
		FMath::Clamp(Location.Z, WorldBoundaryMin.Z, WorldBoundaryMax.Z)
	);
}

void APopulationMeshActor::DrawDebugBoundaries() const {

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

FVector APopulationMeshActor::GetMovementBoundaries(bool& bOutMinBoundary, FVector& OutMin, FVector& OutMax) {

	bOutMinBoundary = bHasValidBoundaries;
	OutMin = WorldBoundaryMin;
	OutMax = WorldBoundaryMax;
	return (WorldBoundaryMin + WorldBoundaryMax) * 0.5f;
}

// Called every frame
void APopulationMeshActor::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

	// Updates if there is a Simulation Controller
	if (!SimulationController) {

		return;
	}

	if (bIsBitten && PopulationType != EPopulationType::Zombie) {

		CheckForTransformation();
	}

	// Checks if Population values/type has changed
	float CurrentPopulationValue = GetCurrentPopulationValue();
	if (CurrentPopulationValue != PreviousPopulationValue || PopulationType != PreviousPopulationType) {

		UpdateMeshBasedOnPopulation();
		PreviousPopulationValue = CurrentPopulationValue;
		PreviousPopulationType = PopulationType;
	}

	// Bitten Characters remain stationary until they transform
	if (PopulationType == EPopulationType::Bitten) {
		// Bitten characters don't move - they remain stationary until transformation
		return;
	}

	// Handle zombie behavior - choose between teleportation or traditional movement/biting
	if (PopulationType == EPopulationType::Zombie) {

		if (bEnableTeleportation) {

			// Use teleportation behavior like ZombieGirlActor
			HandleZombieTeleportation(DeltaTime);
		}

		else if (bEnableBiting) {

			// Use traditional zombie biting behavior
			HandleZombieBitingBehavior(DeltaTime);
		}
	}

	// Handle movement behavior for non-zombie types (only susceptible now, since bitten are excluded above)
	if (bShouldWander && PopulationType == EPopulationType::Susceptible) {

		GirlsHandleWanderingMovement(DeltaTime);
	}

	else if (PopulationType == EPopulationType::Zombie && !bEnableTeleportation && CurrentTarget && IsValid(CurrentTarget)) {

		HandleZombieTargetedMovement(DeltaTime);
	}

	else if (PopulationType == EPopulationType::Zombie && !bEnableTeleportation && bShouldWander) {

		GirlsHandleWanderingMovement(DeltaTime);
	}
}

void APopulationMeshActor::SetupMeshComponent() {

	// Show/hide appropriate Mesh Component
	SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh);
	StaticMeshComponent->SetVisibility(!bUseSkeletalMesh);

	// Set base Static Mesh if using Static Mesh Component
	if (!bUseSkeletalMesh && BaseMesh) {

		StaticMeshComponent->SetStaticMesh(BaseMesh);
	}
}

void APopulationMeshActor::UpdateMeshBasedOnPopulation() {

	if (!bUseSkeletalMesh) {

		return;
	}

	USkeletalMesh* MeshToUse = nullptr;
	UAnimBlueprint* AnimBPToUse = nullptr;

	// Switch case for selecting correct Mesh and Animation based on population Type
	switch (PopulationType) {

	case EPopulationType::Susceptible:
		MeshToUse = SusceptibleMesh;
		AnimBPToUse = SusceptibleAnimBP;
		break;

	case EPopulationType::Bitten:
		MeshToUse = BittenMesh;
		AnimBPToUse = BittenAnimBP;
		break;

	case EPopulationType::Zombie:
		MeshToUse = ZombieMesh;
		AnimBPToUse = ZombieAnimBP;
		break;
	}

	// Apply Mesh
	if (MeshToUse && SkeletalMeshComponent) {

		SkeletalMeshComponent->SetSkeletalMesh(MeshToUse);

		// Apply Animation
		if (AnimBPToUse) {

			SkeletalMeshComponent->SetAnimInstanceClass(AnimBPToUse->GetAnimBlueprintGeneratedClass());
		}
	}
}

float APopulationMeshActor::GetCurrentPopulationValue() const {

	if (!SimulationController) {

		return 0.0f;
	}

	// Switch Case for returning Correct Stock based on Population Type
switch (PopulationType) {

		case EPopulationType::Susceptible:
			return SimulationController->Susceptible;

		case EPopulationType::Bitten:
			return SimulationController->Bitten;

		case EPopulationType::Zombie:
			return SimulationController->Zombies;

		default:
			return 0.0f;
	}
}

void APopulationMeshActor::FindSimulationController() {

	// Find the first Simulation Controller in the world
	if (UWorld* World = GetWorld()) {

		for (TActorIterator<ASimulationController> ActorIterator(World); ActorIterator; ++ActorIterator) {

			SimulationController = *ActorIterator;
			break;
		}

		if (!SimulationController) {

			UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Could Not Find Simulation Controller"));
		}
	}
}

bool APopulationMeshActor::CanBeBitten() const {

	return bCanBeBitten && !bIsBitten && PopulationType == EPopulationType::Susceptible;
}

void APopulationMeshActor::GetBitten(float CurrentSimulationTime) {

	if (!CanBeBitten())
		return;

	bIsBitten = true;
	BittenTimestamp = CurrentSimulationTime;
	bCanBeBitten = false;

	PopulationType = EPopulationType::Bitten;
	UpdateMeshBasedOnPopulation();

	UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Actor %s has been bitten at simulation time %f "),
		* GetName(), CurrentSimulationTime);

}

bool APopulationMeshActor::ShouldTransformToZombie(float CurrentSimulationTime) const {

	if (!bIsBitten || BittenTimestamp < 0.0f)
		return false;

	float DaysSinceBite = CurrentSimulationTime - BittenTimestamp;
	return DaysSinceBite >= 15.0f;
}

void APopulationMeshActor::TransformToZombie() {

	if (PopulationType == EPopulationType::Zombie)
		return;

	PopulationType = EPopulationType::Zombie;
	UpdateMeshBasedOnPopulation();

	// Reset teleportation timer when transforming to zombie
	TeleportTimer = 0.0f;

	// Disable regular wandering for zombies, they use teleportation instead
	if (bEnableTeleportation) {

		bShouldWander = false;
	}

	UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Actor %s has been transformed into a zombie and will now teleport"), *GetName());
}

bool APopulationMeshActor::IsValidBiteTarget() const {

	return PopulationType == EPopulationType::Susceptible && CanBeBitten();
}

void APopulationMeshActor::CheckForTransformation() {

	if (!SimulationController)
		return;

	float CurrentSimulationTime = static_cast<float>(SimulationController->TimeStepsFinished);

	if (ShouldTransformToZombie(CurrentSimulationTime)) {

		TransformToZombie();
	}
}

// Zombie teleportation behavior methods (Same code from ZombieGirlActor)
void APopulationMeshActor::HandleZombieTeleportation(float DeltaTime) {

	if (!SimulationController || PopulationType != EPopulationType::Zombie)
		return;

	// Update teleport timer
	TeleportTimer += DeltaTime;
	LastBiteTime += DeltaTime;

	// Check if it's time to teleport
	if (TeleportTimer >= TeleportInterval) {

		// Find a random girl to teleport to
		APopulationMeshActor* RandomTarget = FindRandomBiteTarget();
		
		if (RandomTarget) {
			
			// Teleport to the target
			TeleportToTarget(RandomTarget);
			
			// Attempt to bite after teleporting
			AttemptBiteAfterTeleport(RandomTarget);
			
			if (bEnableDebugTeleport) {

				UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor Zombie: %s teleported to bite target %s"), 
					*GetName(), *RandomTarget->GetName());
			}
		}

		else {

			if (bEnableDebugTeleport) {

				UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor Zombie: %s could not find any bite targets"), *GetName());
			}
		}

		// Reset teleport timer
		TeleportTimer = 0.0f;
	}
}

APopulationMeshActor* APopulationMeshActor::FindRandomBiteTarget() {

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

void APopulationMeshActor::TeleportToTarget(APopulationMeshActor* Target) {

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

void APopulationMeshActor::AttemptBiteAfterTeleport(APopulationMeshActor* Target) {

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

			UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor Zombie: %s successfully bit %s after teleportation at simulation time %f"),
				*GetName(), *Target->GetName(), CurrentSimulationTime);
		}
	}
}

// Original zombie biting behavior methods (for when teleportation is disabled -- But then will not work together with simulation timestep)
void APopulationMeshActor::HandleZombieBitingBehavior(float DeltaTime) {

	if (!SimulationController || PopulationType != EPopulationType::Zombie)
		return;

	LastBiteTime += DeltaTime;

	if (LastBiteTime < BiteCooldown)
		return;

	// Find a target if we don't have one
	if (!CurrentTarget || !IsValid(CurrentTarget) || !CurrentTarget->IsValidBiteTarget()) {

		CurrentTarget = FindNearestBiteTarget();
	}
}

void APopulationMeshActor::HandleZombieTargetedMovement(float DeltaTime) {

	if (!CurrentTarget || PopulationType != EPopulationType::Zombie) 
		return;

	FVector TargetLocation = CurrentTarget->GetActorLocation();
	FVector MyLocation = GetActorLocation();
	float DistanceToTarget = FVector::Dist2D(MyLocation, TargetLocation);

	// Move toward target if not in bite range
	if (DistanceToTarget > BiteRange) {

		FVector DirectionToTarget = (TargetLocation - MyLocation).GetSafeNormal();
		FVector NewLocation = MyLocation + (DirectionToTarget * MovementSpeed * DeltaTime);

		SetActorLocation(NewLocation);

		// Face the target
		FRotator NewRotation = DirectionToTarget.Rotation();
		SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
	}

	else {

		// We're in range, attempt to bite
		TryToBiteNearbyTargets();
	}
}

void APopulationMeshActor::TryToBiteNearbyTargets() {

	if (!SimulationController || PopulationType != EPopulationType::Zombie)
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

			UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor Zombie: %s bit %s at simulation time %f"),
				*GetName(), *PotentialTarget->GetName(), CurrentSimulationTime);

			// Only bite one target per attempt
			break;
		}
	}
}

APopulationMeshActor* APopulationMeshActor::FindNearestBiteTarget() {

	if (PopulationType != EPopulationType::Zombie)
		return nullptr;

	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	FVector MyLocation = GetActorLocation();
	APopulationMeshActor* NearestTarget = nullptr;
	float NearestDistance = BiteSearchRadius;

	for (TActorIterator<APopulationMeshActor> ActorIterator(World); ActorIterator; ++ActorIterator) {

		APopulationMeshActor* PotentialTarget = *ActorIterator;

		if (!PotentialTarget || PotentialTarget == this)
			continue;

		if (!CanBiteTarget(PotentialTarget))
			continue;

		float Distance = FVector::Dist2D(MyLocation, PotentialTarget->GetActorLocation());

		if (Distance < NearestDistance) {

			NearestDistance = Distance;
			NearestTarget = PotentialTarget;
		}
	}

	return NearestTarget;
}

bool APopulationMeshActor::CanBiteTarget(APopulationMeshActor* Target) const {

	if (!Target || PopulationType != EPopulationType::Zombie)
		return false;

	return Target->IsValidBiteTarget();
}

void APopulationMeshActor::GirlsHandleWanderingMovement(float DeltaTime) {

	FVector CurrentLocation = GetActorLocation();

	// Update direction change timer
	WanderTimer += DeltaTime;
	DirectionChangeTimer += DeltaTime;
	
	// Handle boundary turning behavior
	if (bTurningAroundFromBoundary) {

		BoundaryTurnTimer += DeltaTime;
		if (BoundaryTurnTimer >= 1.0f) {
			// Turn for 1 second
			bTurningAroundFromBoundary = false;
			BoundaryTurnTimer = 0.0f;
		}
	}

	// Check if we're near a boundary and need to turn around
	if (bHasValidBoundaries && IsNearBoundary(CurrentLocation, BoundaryBuffer) && !bTurningAroundFromBoundary) {

		// Get direction to turn away from boundary
		FVector CurrentDirection = FVector(
			FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
			FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
			0.0f
		);
		
		FVector AvoidanceDirection = GetBoundaryAvoidanceDirection(CurrentLocation, CurrentDirection);
		WanderDirection = FMath::RadiansToDegrees(FMath::Atan2(AvoidanceDirection.Y, AvoidanceDirection.X));
		
		bTurningAroundFromBoundary = true;
		BoundaryTurnTimer = 0.0f;
		WanderTimer = 0.0f; // Reset wander timer
		
		UE_LOG(LogTemp, Warning, TEXT("Girl %s turning away from boundary"), *GetName());
	}

	// Normal direction changes when not avoiding boundaries
	else if (!bTurningAroundFromBoundary && WanderTimer >= FMath::RandRange(3.0f, 7.0f)) {

		WanderDirection = FMath::RandRange(0.0f, 360.0f);
		WanderTimer = 0.0f;
	}

	// Calculate movement direction
	FVector DirectionVector = FVector(
		FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
		FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
		0.0f
	);

	// Calculate new position
	FVector NewLocation = CurrentLocation + (DirectionVector * MovementSpeed * DeltaTime);

	// Clamp to boundaries if enabled
	if (bHasValidBoundaries) {

		NewLocation = ClampToBoundaries(NewLocation);
		
		// If the clamped position is different, we hit a boundary - force direction change
		if (!NewLocation.Equals(CurrentLocation + (DirectionVector * MovementSpeed * DeltaTime), 10.0f)) {

			WanderDirection = FMath::RandRange(0.0f, 360.0f);
			bTurningAroundFromBoundary = true;
			BoundaryTurnTimer = 0.0f;
		}
	}

	// Apply movement
	SetActorLocation(NewLocation);
	LastValidPosition = NewLocation;

	// Rotate to face movement direction
	FRotator NewRotation = DirectionVector.Rotation();
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));

	// Draw debug boundaries if enabled
	if (bDrawDebugBoundaries) {

		DrawDebugBoundaries();
	}
}

void APopulationMeshActor::OnDeath() const {
	TArray < AActor* > mSimControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASimulationController::StaticClass(), mSimControllers);

	

	AActor* mSimController = mSimControllers[0];

	ASimulationController* SimController = Cast <ASimulationController>(mSimController);
	if (SimController != NULL) {
		if (PopulationType == EPopulationType::Zombie) {
			SimController->Zombies--;
		}
		if (PopulationType == EPopulationType::Susceptible) {
			SimController->Susceptible--;
		}
		if (PopulationType == EPopulationType::Bitten) {
			SimController->Bitten--;
			
			float CurrentSimulationTime = static_cast<float>(SimulationController->TimeStepsFinished);
			int ThisDaysLeft = CurrentSimulationTime - BittenTimestamp;

			for (auto& b : SimController->conveyor) {
				if (static_cast<int>(b.remainingDays) == static_cast<int>(ThisDaysLeft)) {
					b.amountOfPeople--;
				}
			}

		}
	}

	

}