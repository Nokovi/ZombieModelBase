// Copyright University of Inland Norway


#include "HealthInterface.h"

// Add default functionality here for any IHealthInterface functions that are not pure virtual.

void IHealthInterface::ImpartDamage(float DamageInput, UObject* Damager)
{
	HealthPoints -= DamageInput;
	ImpartDamageBlueprintEvent(DamageInput, Damager);


	//Destroy actor if health empty.
	if (HealthPoints <= 0) {
		CharacterDeath('0');
	}
}


void IHealthInterface::ImpartDamage(float DamageInput, UObject* Damager, char DeathCase)
{
	HealthPoints -= DamageInput;
	ImpartDamageBlueprintEvent(DamageInput, Damager);


	//Destroy actor if health empty.
	if (HealthPoints <= 0) {
		CharacterDeath(DeathCase);
	}
}

void IHealthInterface::ImpartDamageBlueprintEvent(float DamageInput, UObject* Damager)
{
	//no need to write anything here, probably.
}

void IHealthInterface::CharacterDeath(char DeathCase)
{
	switch (DeathCase)
		case '0': {
		if (AActor* ThisActor = Cast<AActor>(this)) {
			ThisActor->Destroy();
		}
	}
}
