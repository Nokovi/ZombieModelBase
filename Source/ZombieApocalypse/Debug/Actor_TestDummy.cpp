// Copyright University of Inland Norway


#include "Actor_TestDummy.h"

// Sets default values
AActor_TestDummy::AActor_TestDummy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AActor_TestDummy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AActor_TestDummy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AActor_TestDummy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

