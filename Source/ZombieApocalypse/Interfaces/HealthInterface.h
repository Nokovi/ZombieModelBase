// Copyright University of Inland Norway

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthInterface.generated.h"

/// <summary>
///  This class does not need to be modified. Do not inherit this. This is purely for Unreal.
/// </summary>
UINTERFACE(MinimalAPI, Blueprintable)
class UHealthInterface : public UInterface
{
	GENERATED_BODY()
};

/// <summary>
/// INHERIT THIS!!!!
/// </summary>
class ZOMBIEAPOCALYPSE_API IHealthInterface
{
public:
	GENERATED_BODY()

	float HealthPoints = 100.f;


	/// <summary>
	/// Damage function.
	/// </summary>
	/// <param name="DamageInput">How much damage should be imparted.</param>
	/// <param name="Damager">Reference to projectile owner or projectile itself, could be useful.</param>
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	 void ImpartDamage(float DamageInput, UObject* Damager);
	 virtual void ImpartDamage_Implementation(float DamageInput, UObject* Damager);
	//if you for some reason want multiple death types, use the bottom one. otherwise meh.
	

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ImpartDamageBlueprintEvent(float DamageInput, UObject* Damager); 
	//^If you want this function to do extra stuff in blueprint (f.ex. visuals), use this.^

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void CharacterDeath();
	virtual void CharacterDeath_Implementation();

	

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	float GetHealthPoints();
	float GetHealthPoints_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void SetHealthPoints(float HP);
	void SetHealthPoints_Implementation(float HP);
};
