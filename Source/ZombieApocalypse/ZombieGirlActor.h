#pragma once

#include "CoreMinimal.h"
#include "PopulationMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimBlueprint.h"
#include "ZombieGirlActor.generated.h"

UCLASS()
class ZOMBIEAPOCALYPSE_API AZombieGirlActor : public APopulationMeshActor {

	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AZombieGirlActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Zombie behavior settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float MovementSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float AttackRange = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	bool bShouldWander = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Behavior")
	float WanderRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Behavior")
	float BiteRange = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Behavior")
	float BiteCooldown = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Behavior")
	bool bEnableBiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Behavior")
	float BiteSearchRadius = 300.0f;

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
	bool bEnableZombieEffects = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
	FLinearColor ZombieTint = FLinearColor(0.8f, 1.0f, 0.8f, 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Biting")
	void TryToBiteNearbyTargets();

	UFUNCTION(BlueprintCallable, Category = "Biting")
	APopulationMeshActor* FindNearestBiteTarget();

	UFUNCTION(BlueprintCallable, Category = "Biting")
	bool CanBiteTarget(APopulationMeshActor* Target) const;

private:
	void UpdateZombieMesh();
	float GetZombiePopulation() const;
	void SetupZombieComponents();
	void HandleBitingBehavior(float DeltaTime);

	// Zombie-specific tracking
	float PreviousZombieCount = 0.0f;
	FVector InitialLocation;
	float WanderTimer = 0.0f;
	float WanderDirection = 0.0f;

	float LastBiteTime = 0.0f;
	APopulationMeshActor* CurrentTarget = nullptr;
};
