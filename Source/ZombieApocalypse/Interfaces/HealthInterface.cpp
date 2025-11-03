// Copyright University of Inland Norway


#include "HealthInterface.h"

// Add default functionality here for any IHealthInterface functions that are not pure virtual.

void IHealthInterface::ImpartDamage(float DamageInput, UObject* Damager)
{
	HealthPoints -= DamageInput;
	ImpartDamageBlueprintEvent();
}

void IHealthInterface::ImpartDamageBlueprintEvent()
{
}
