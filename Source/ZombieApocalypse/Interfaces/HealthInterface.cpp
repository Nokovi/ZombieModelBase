// Copyright University of Inland Norway


#include "HealthInterface.h"
#include "../PopulationMeshActor.h"


// Add default functionality here for any IHealthInterface functions that are not pure virtual.

void IHealthInterface::ImpartDamage_Implementation(float DamageInput, UObject* Damager)
{
	HealthPoints -= DamageInput;
	Execute_ImpartDamageBlueprintEvent(Cast<UObject>(this), DamageInput, Damager);

	GEngine->AddOnScreenDebugMessage(2, 2.0f, FColor::Red, FString::Printf(TEXT("Working! %f"), HealthPoints));

	if (HealthPoints <= 0) {
		Execute_CharacterDeath(Cast<UObject>(this));
	}
}

void IHealthInterface::CharacterDeath_Implementation()
{
	if (APopulationMeshActor* ThisPopulationMeshActor = Cast<APopulationMeshActor>(this)) {
		ThisPopulationMeshActor->OnDeath();
	}
	if (AActor* ThisActor = Cast<AActor>(this)) {
		ThisActor->Destroy();
		GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Red, FString::Printf(TEXT("Died!")), true);
	}
}

float IHealthInterface::GetHealthPoints_Implementation()
{
	return HealthPoints;
}

void IHealthInterface::SetHealthPoints_Implementation(float HP)
{
	HealthPoints = HP;
}
