#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "SimulationController.h"
#include "PopulationMeshActor.generated.h"


UENUM(BlueprintType)
enum class EPopulationType : uint8 {

	Susceptible UMETA(DisplayName = "Susceptible"),
	Bitten UMETA(DisplayName = "Bitten"),
	Zombie UMETA(DisplayName = "Zombie")
};

UCLASS()
class ZOMBIEAPOCALYPSE_API APopulationMeshActor : public AActor
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

	UPROPERTY(BlueprintReadWrite, Category = "Bite Management System")
	bool bIsBitten = false;

	UPROPERTY(BlueprintReadWrite, Category = "Bite Management System")
	float BittenTimestamp = -1, 0f;

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

private:

	void UpdateMeshBasedOnPopulation();
	float GetCurrentPopulationValue() const;
	void SetupMeshComponent();
	void FindSimulationController();

	// Detecting changes
	float PreviousPopulationValue;
	EPopulationType PreviousPopulationType;

	void CheckForTransformation();
};
