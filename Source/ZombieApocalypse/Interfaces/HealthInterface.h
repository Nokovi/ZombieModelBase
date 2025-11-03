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

	float HealthPoints = 100;


	/// <summary>
	/// Damage function.
	/// </summary>
	/// <param name="DamageInput">How much damage should be imparted.</param>
	/// <param name="Damager">Reference to projectile owner or projectile itself, could be useful.</param>
	UFUNCTION()
	
	
	void ImpartDamage(float DamageInput, UObject* Damager);
	//if you for some reason want multiple death types, use the bottom one. otherwise meh.
	void ImpartDamage(float DamageInput, UObject* Damager, char DeathCase);
	

private:
	UFUNCTION(BlueprintNativeEvent)
	virtual void ImpartDamageBlueprintEvent(float DamageInput, UObject* Damager); 
	//^If you want this function to do extra stuff in blueprint (f.ex. visuals), use this.^

	UFUNCTION(BlueprintCallable)
	virtual void CharacterDeath(char DeathCase);
};
