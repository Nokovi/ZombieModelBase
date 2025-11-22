#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "interfaces/HealthInterface.h"
#include "SimulationController.h"
#include "PopulationMeshActor.generated.h"


UENUM(BlueprintType)
enum class EPopulationType : uint8 {

	Susceptible UMETA(DisplayName = "Susceptible"),
	Bitten UMETA(DisplayName = "Bitten"),
	Zombie UMETA(DisplayName = "Zombie")
};

UCLASS()
class ZOMBIEAPOCALYPSE_API APopulationMeshActor : public AActor, public IHealthInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APopulationMeshActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Skibidi Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Components)
	UCapsuleComponent* CapsuleComponent;



	// Skibidi Meshes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Assets")
	USkeletalMesh* SusceptibleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Assets")
	USkeletalMesh* BittenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Assets")
	USkeletalMesh* ZombieMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Assets")
	UStaticMesh* BaseMesh;

	// Skibidi Population Type Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Population Settings")
	EPopulationType PopulationType;

	// Simulation Controller Reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	ASimulationController* SimulationController;

	// Skibidi Visual Effects Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
	float ScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
	bool bUseSkeletalMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
	bool bAutoFindSimulationController = true;

	// Skibidi Animation Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimBlueprint* SusceptibleAnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimBlueprint* BittenAnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimBlueprint* ZombieAnimBP;

	// Zombie behavior settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float MovementSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float AttackRange = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	bool bShouldWander = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float WanderRadius = 500.0f;

	// Movement boundary settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Boundaries")
	bool bUseCustomBoundaries = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Boundaries", meta = (EditCondition = "bUseCustomBoundaries"))
	FVector CustomBoundaryMin = FVector(-5000.0f, -5000.0f, -1000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Boundaries", meta = (EditCondition = "bUseCustomBoundaries"))
	FVector CustomBoundaryMax = FVector(5000.0f, 5000.0f, 1000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Boundaries")
	float BoundaryBuffer = 200.0f; // Distance from boundary before turning around

	// Debug settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugBoundaries = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bite Management System")
	bool bIsBitten = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bite Management System")
	float BittenTimestamp = -1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Bite Management System")
	bool bCanBeBitten = true;

	UFUNCTION(BlueprintCallable, Category = "Bite Management System")
	bool CanBeBitten() const;

	UFUNCTION(BlueprintCallable, Category = "Bite Management System")
	void GetBitten(float CurrentSimulationTime);

	UFUNCTION(BlueprintCallable, Category = "Bite Management System")
	bool ShouldTransformToZombie(float CurrentSimulationTime) const;

	UFUNCTION(BlueprintCallable, Category = "Bite Management System")
	void TransformToZombie();

	UFUNCTION(BlueprintCallable, Category = "Bite Management System")
	bool IsValidBiteTarget() const;

	// Movement boundary functions
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector GetMovementBoundaries(bool& bOutMinBoundary, FVector& OutMin, FVector& OutMax);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool IsWithinBoundaries(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector ClampToBoundaries(const FVector& Location) const;


	void OnDeath() const;

private:

	void UpdateMeshBasedOnPopulation();
	float GetCurrentPopulationValue() const;
	void SetupMeshComponent();
	void FindSimulationController();
	void GirlsHandleWanderingMovement(float DeltaTime);


	// Movement boundary helpers
	void CalculateWorldBoundaries();
	FVector GetBoundaryAvoidanceDirection(const FVector& CurrentLocation, const FVector& CurrentDirection);
	bool IsNearBoundary(const FVector& Location, float Buffer = 0.0f) const;
	void DrawDebugBoundaries() const;

	FVector InitialLocation;
	float WanderTimer = 0.0f;
	float WanderDirection = 0.0f;

	APopulationMeshActor* CurrentTarget = nullptr;

	// Movement boundaries
	FVector WorldBoundaryMin;
	FVector WorldBoundaryMax;
	bool bHasValidBoundaries = false;

	// Movement state
	FVector LastValidPosition;
	float DirectionChangeTimer = 0.0f;
	bool bTurningAroundFromBoundary = false;
	float BoundaryTurnTimer = 0.0f;

	// Detecting changes
	float PreviousPopulationValue;
	EPopulationType PreviousPopulationType;

	void CheckForTransformation();
};
