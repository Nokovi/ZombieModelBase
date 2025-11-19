// Copyright University of Inland Norway


#include "PopulationMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
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

	// Hides Static Mesh if Skeletal Mesh is used
	StaticMeshComponent->SetVisibility(!bUseSkeletalMesh);
	SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh);

	MovementSpeed = 250.0f;
	AttackRange = 75.0f;
	bShouldWander = true;
	WanderRadius = 50000.0f; // Greatly increased wander radius for free movement

	// Disable boundary restrictions for free movement
	BoundaryBuffer = 0.0f;
	bUseCustomBoundaries = false;
	bHasValidBoundaries = false; // Disable boundary system entirely

	// Initialize timers
	WanderTimer = 0.0f;
	WanderDirection = FMath::RandRange(0.0f, 360.0f);
	DirectionChangeTimer = 0.0f;
	bTurningAroundFromBoundary = false;
	BoundaryTurnTimer = 0.0f;
}

// Called when the game starts or when spawned
void APopulationMeshActor::BeginPlay()
{
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

	UE_LOG(LogTemp, Warning, TEXT("ZombieGirlActor: Calculated boundaries - Min: %s, Max: %s"),
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
void APopulationMeshActor::Tick(float DeltaTime)
{
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

	// Handle movement behavior - removed target dependency for free movement
	if (bShouldWander) {
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

	UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Actor %s has been bitten at siumlation time %f "),
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

	UE_LOG(LogTemp, Warning, TEXT("PopulationMeshActor: Actor % s has been transformed into a zombie"), *GetName());
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

void APopulationMeshActor::GirlsHandleWanderingMovement(float DeltaTime) {

	FVector CurrentLocation = GetActorLocation();

	// Update direction change timer
	WanderTimer += DeltaTime;
	DirectionChangeTimer += DeltaTime;

	// Simplified direction changes - no boundary restrictions
	if (WanderTimer >= FMath::RandRange(2.0f, 5.0f)) {
		WanderDirection = FMath::RandRange(0.0f, 360.0f);
		WanderTimer = 0.0f;
	}

	// Calculate movement direction
	FVector DirectionVector = FVector(
		FMath::Cos(FMath::DegreesToRadians(WanderDirection)),
		FMath::Sin(FMath::DegreesToRadians(WanderDirection)),
		0.0f
	);

	// Calculate new position - no boundary clamping
	FVector NewLocation = CurrentLocation + (DirectionVector * MovementSpeed * DeltaTime);

	// Move freely without boundary restrictions
	SetActorLocation(NewLocation);

	// Rotate to face movement direction
	FRotator NewRotation = DirectionVector.Rotation();
	SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}