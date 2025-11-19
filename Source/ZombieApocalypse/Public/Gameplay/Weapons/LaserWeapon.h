// Copyright University of Inland Norway

#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "LaserWeapon.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIEAPOCALYPSE_API ALaserWeapon : public AWeaponBase
{
	GENERATED_BODY()
private:
	ALaserWeapon();
	void PrimaryFireExec();
	void SecondaryFireRepeating();
	void SecondaryFireStop();
public:

	//These are used for the particle system to figure out where to put the beam and splatter effect.
	UPROPERTY(BlueprintReadOnly, Category = "VFX")
	FVector StartLocation;
	UPROPERTY(BlueprintReadOnly, Category = "VFX")
	FVector ImpactLocation;


	virtual void PrimaryFire() override;
	virtual void SecondaryFire() override;

	virtual void AttachWeapon(ACharacter* Owner, UWeaponUIWidget* HUD) override;

	float BaseChargeTime = 1.0f;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "VFX")
	void SpawnNiagaraBP(); //UFunction linked to blueprint that spawns particle effect.
	void SpawnNiagaraBP_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "VFX")
	void SpawnNiagaraBPConstant(); //UFunction linked to blueprint that spawns particle effect.
	void SpawnNiagaraBPConstant_Implementation();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, category = "VFX")
	void UpdateNiagaraBPConstant(); //UFunction linked to blueprint that spawns particle effect.
	void UpdateNiagaraBPConstant_Implementation();

	TArray<FHitResult> GetViewTarget();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VFX")
	UNiagaraSystem* NiagaraSystem;
	
	
	
	virtual void Tick(float DeltaTime) override;


	bool bHasPenUpgrade = false;



	bool bFiringSecondary = false;
	bool bSNiagaraExist = false;
};
