// Copyright University of Inland Norway


//stolen from a year 1 project I worked on.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../../SimulationHUD.h"
#include "../../WeaponUIWidget.h"
#include "WeaponBase.generated.h"

UCLASS()
class ZOMBIEAPOCALYPSE_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
// Sets default values for this actor's properties
	AWeaponBase();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void DoOnPurchase();
	virtual void PrimaryFire();
	virtual void SecondaryFire();
	void Reload();
	virtual void AttachWeapon(ACharacter* NOwner, UWeaponUIWidget* HUD);
	bool bIsReloading = false;

	//Max ammo.
	unsigned int AmmoMax = 50;
	//Current Ammo.
	unsigned int Ammo = 50;
	//Weapon clip size before reloading.
	unsigned int AmmoClip = 50;
	//Is the weapon fully automatic?
	bool bIsAutomatic;


	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Visual")
	UStaticMesh* Mesh = nullptr;
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Visual")
	UAnimMontage* TriggerAnimation = nullptr;
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Visual")
	UAnimMontage* AltTriggerAnimation = nullptr;

	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Audio")
	USoundBase* FireSound = nullptr;
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Audio")
	USoundBase* AltFireSound = nullptr;
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Audio")
	USoundBase* NoAmmoSound = nullptr;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//Variables for weapon.

	void ResetFireState();
	void ResetReload();
	int CanTrigger();


	FVector WeaponOffset;


	//Rate of fire as defined by time interval between shots.
	float FireInterval = 0.5f;
	//
	float ReloadTime = 2.0f;

	// Checks for reload and

	bool bCanFire = true;

	UWorld* World;



	bool bIsAttached;


	ACharacter* PlayerCharacter;
	UWeaponUIWidget* PlayerHUD;
};
