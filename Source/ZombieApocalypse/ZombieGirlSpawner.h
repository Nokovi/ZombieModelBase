#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieGirlActor.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "ZombieGirlSpawner.generated.h"

UCLASS()
class ZOMBIEAPOCALYPSE_API AZombieGirlSpawner : public AActor {

	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AZombieGirlSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Spawn Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	int32 NumberToSpawn = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpacingBetweenActors = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	int32 ActorsPerRow = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bAutoSpawnOnBeginPlay = true;

	// Zombie Asset Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Assets")
	USkeletalMesh* ZombieMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie Assets")
	UAnimBlueprint* ZombieAnimBP;

	// Actor Class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<AZombieGirlActor> ZombieGirlActorClass;

	// Debug Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableDebugLogging = true;

	// Spawning Functions
	UFUNCTION(CallInEditor, Category = "Spawning")
	void SpawnZombies();

	UFUNCTION(CallInEditor, Category = "Spawning")
	void ClearSpawnedZombies();

private:
	// Tracking spawned zombies
	UPROPERTY()
	TArray<AZombieGirlActor*> SpawnedZombies;

	void ConfigureSpawnedZombie(AZombieGirlActor* SpawnedZombie);
	void CleanupSpawnedActors();
	FVector CalculateSpawnLocation(int32 Index) const;
};
