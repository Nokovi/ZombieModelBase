#pragma once

#include "CoreMinimal.h"
#include "PopulationMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/World.h"
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

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
	bool bEnableZombieEffects = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Effects")
	FLinearColor ZombieTint = FLinearColor(0.8f, 1.0f, 0.8f, 1.0f);

	// NEW: Teleportation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Teleportation")
	float TeleportInterval = 1.0f; // Teleport every second

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Teleportation")
	float TeleportRange = 150.0f; // How close to teleport near the target

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Teleportation")
	bool bEnableTeleportation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Teleportation")
	bool bEnableDebugTeleport = true;

private:
	void UpdateZombieMesh();
	float GetZombiePopulation() const;
	void SetupZombieComponents();
	
	// NEW: Teleportation methods (replacing movement)
	void HandleZombieTeleportation(float DeltaTime);
	APopulationMeshActor* FindRandomBiteTarget();
	void TeleportToTarget(APopulationMeshActor* Target);
	void AttemptBiteAfterTeleport(APopulationMeshActor* Target);

	// Zombie-specific tracking
	float PreviousZombieCount = 0.0f;
	
	// NEW: Teleportation tracking (replacing movement variables)
	float TeleportTimer = 0.0f;
	float LastBiteTime = 0.0f;
};

