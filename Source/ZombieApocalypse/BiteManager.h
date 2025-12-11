#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PopulationMeshActor.h"
#include "SimulationController.h"
#include "BiteManager.generated.h"

USTRUCT(BlueprintType)
struct FBiteRecord {

	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<APopulationMeshActor> BittenActor;

	UPROPERTY(BlueprintReadWrite)
	float BiteTime;

	UPROPERTY(BlueprintReadWrite)
	int32 ScheduledTransformationDay;

	FBiteRecord() {

		BittenActor = nullptr;
		BiteTime = 5.0f;
	}

	FBiteRecord(APopulationMeshActor* Actor, float Time) {

		BittenActor = Actor;
		BiteTime = Time;
	}
};

UCLASS()
class ZOMBIEAPOCALYPSE_API ABiteManager : public AActor {

	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABiteManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Bite management settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Management")
	float TransformationDays = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bite Management")
	bool bEnableDebugLogging = true;

	// Simulation controller reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	ASimulationController* SimulationController;

	UPROPERTY(BlueprintReadOnly, Category = "Integration")
	bool bSyncWithSimulationController = true;

	UPROPERTY(BlueprintReadOnly, Category = "Integration")
	float SimulationControllerBittenRatio = 0.0f; // Track what % of bitten population should transform

	// Bite management functions
	UFUNCTION(BlueprintCallable, Category = "Bite Management")
	void RegisterBite(APopulationMeshActor* BittenActor, float BiteTime);

	UFUNCTION(BlueprintCallable, Category = "Bite Management")
	void CheckForTransformations();

	UFUNCTION(BlueprintCallable, Category = "Bite Management")
	int32 GetActiveBiteCount() const;

private:
	// Bite tracking
	UPROPERTY()
	TArray<FBiteRecord> ActiveBites;

	int32 NextAvailableTransformationDay = 15;

	void FindSimulationController();
	void CleanupInvalidBites();

	void SchedulePendingTransformations();

	// Track previous simulation bitten count for transformation detection
	float LastKnownSimulationBitten = 0.0f;

	// Helper methods
	void TransformOldestScheduledActors(int32 ActorsToTransform, int32 CurrentDay);
	void CheckForIndividualTransformations(int32 CurrentDay);

};

