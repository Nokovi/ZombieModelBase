// Copyright University of Inland Norway

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PopulationMeshActor.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "GirlSpawner.generated.h"

UCLASS()
class ZOMBIEAPOCALYPSE_API AGirlSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGirlSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 NumberToSpawn = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpacingBetweenActors = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 ActorsPerRow = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<APopulationMeshActor> GirlActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bAutoSpawnOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Girl Assets")
	USkeletalMesh* GirlMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Girl Assets")
	UAnimBlueprint* GirlAnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Girl Assets")
	bool bEnableDebugLogging = true;

	// Manual Spawn if needed
	UFUNCTION(CallInEditor, Category = "Spawner")
	void SpawnGirls();

	UFUNCTION(CallInEditor, Category = "Spawner")
	void ClearSpawnedGirls();

private:

	UPROPERTY()
	TArray<APopulationMeshActor*> SpawnedGirls;

	void CleanupSpawnedActors();
	void ConfigureSpawnedGirl(APopulationMeshActor* SpawnedGirl);
	FVector CalculateSpawnLocation(int32 Index) const;
};
