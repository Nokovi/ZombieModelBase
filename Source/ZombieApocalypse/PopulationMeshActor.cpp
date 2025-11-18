// Copyright University of Inland Norway


#include "PopulationMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

