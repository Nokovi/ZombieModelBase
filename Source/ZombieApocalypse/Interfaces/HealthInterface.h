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
	GENERATED_BODY()

	float HealthPoints = 100;

	UFUNCTION()
	void ImpartDamage(float DamageInput, UObject* Damager);

	UFUNCTION(BlueprintNativeEvent)
	virtual void ImpartDamageBlueprintEvent(); //If you want this function to do extra stuff in blueprint (f.ex. visuals), use this.

};
