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

	FBiteRecord() {

		BittenActor = nullptr;
		BiteTime = 0.0f;
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

	void FindSimulationController();
	void CleanupInvalidBites();
};