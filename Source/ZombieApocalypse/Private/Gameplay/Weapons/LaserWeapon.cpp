// Copyright University of Inland Norway


#include "Gameplay/Weapons/LaserWeapon.h"
//#include "CPP_Player.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "../Interfaces/HealthInterface.h"
#include <iostream>
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"



//I should really start adding white space so it's not so incredibly dense.
ALaserWeapon::ALaserWeapon()
{
	AmmoMax = 10;
	AmmoClip = 10;
	Ammo = 10;
	FireInterval = 0.1f;
	BaseChargeTime = 1.5f;
	WeaponOffset = { 0.f, 0.f, 50.f };
	PrimaryActorTick.bCanEverTick = true;
}
void ALaserWeapon::PrimaryFire() {
	if (bCanFire && Ammo > 0) {
		World = PlayerCharacter->GetWorld();
		bCanFire = false;
		if (PlayerCharacter != nullptr) {
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, PlayerCharacter->GetActorLocation(), 1, 1, 0.3f);
			FTimerHandle TheOne;
			//delayed fire.
			if (World) World->GetTimerManager().SetTimer(TheOne, this, &ALaserWeapon::PrimaryFireExec, BaseChargeTime, false);
		}

	}
}
/// <summary>
/// Use for when it actually fires the beam. Triple shot might be fun.
/// </summary>
void ALaserWeapon::PrimaryFireExec() {

	TArray<FHitResult> Hit = GetViewTarget();

	if (Hit.IsValidIndex(0)) {
		if (bHasPenUpgrade == true) {
			for (int i = 0; i < Hit.Max(); i++) {
				ImpactLocation = Hit[i].Location;
				if (Hit[i].GetActor()->GetClass()->ImplementsInterface(UHealthInterface::StaticClass()) == true) {

					IHealthInterface* HitObject = Cast<IHealthInterface>(Hit[i].GetActor());
					HitObject->Execute_ImpartDamage(Hit[i].GetActor(), (200 / (i + 1)), Cast<UObject>(PlayerCharacter));

				}
			}
		}
		else { //If the player doesn't have the upgrade, we just stop at the first hit.
			ImpactLocation = Hit[0].Location;
			if (Hit[0].GetActor()->GetClass()->ImplementsInterface(UHealthInterface::StaticClass()) == true) {
				IHealthInterface* HitObject = Cast<IHealthInterface>(Hit[0].GetActor());
				HitObject->Execute_ImpartDamage(Hit[0].GetActor(), (200), Cast<UObject>(PlayerCharacter));

			}
		}
	}
	//SpawnNiagaraBP();
	Ammo--;
	Super::PrimaryFire();
}




void ALaserWeapon::SecondaryFire()
{
	if (bCanFire == true && bFiringSecondary == false) {
		World = PlayerCharacter->GetWorld();

		if (Ammo == AmmoClip) {
			bCanFire = false;
			bFiringSecondary = true;
			FTimerHandle TheOneTwo[30];

			for (unsigned int i = 0; i < 30; i++)
			{
				//repeating timers? Why would I do that?
				if (World) World->GetTimerManager().SetTimer(TheOneTwo[i], this, &ALaserWeapon::SecondaryFireRepeating, (BaseChargeTime + i * 0.1), false);

			}
			FTimerHandle SecondaryStopTimer;
			World->GetTimerManager().SetTimer(SecondaryStopTimer, this, &ALaserWeapon::SecondaryFireStop, (BaseChargeTime + 31 * 0.1), false);
		}
	}
}

void ALaserWeapon::SecondaryFireRepeating() {

	TArray<FHitResult> Hit = GetViewTarget();

	if (bSNiagaraExist == false) {
		//SpawnNiagaraBPConstant();
		bSNiagaraExist = true;
	}

	if (Hit.IsValidIndex(0)) {
		//Can't get pen upgrade.
		ImpactLocation = Hit[0].Location;
		if (Hit[0].GetActor()->GetClass()->ImplementsInterface(UHealthInterface::StaticClass()) == true) {
			IHealthInterface* HitObject = Cast<IHealthInterface>(Hit[0].GetActor());
			HitObject->Execute_ImpartDamage(Hit[0].GetActor(), (200), Cast<UObject>(PlayerCharacter));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, TEXT("Hit"));
		}
	}
	if (Ammo > 0) {
		Ammo--;
	}
	//PlayerHUD->SetUIAmmo(Ammo, AmmoMax);
}
void ALaserWeapon::SecondaryFireStop() {
	//Try not to cause the situation to be misread.
	bFiringSecondary = false;
	bSNiagaraExist = false;
	bCanFire = true;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, TEXT("Wow!"));
}

void ALaserWeapon::AttachWeapon(ACharacter* NOwner, UWeaponUIWidget* HUD)
{
	Super::AttachWeapon(NOwner, HUD);
}

/*
 Blueprintable functions.
*/

void ALaserWeapon::SpawnNiagaraBP_Implementation()
{
	//this is just here so the compiler won't get mad.
}

void ALaserWeapon::SpawnNiagaraBPConstant_Implementation()
{
	//Same as above.
}

void ALaserWeapon::UpdateNiagaraBPConstant_Implementation()
{
	//Again, same as above.
}

/// <summary>
/// Returns line trace of what's in front of the player. (Uses AimTrace channel). Used for weapons and beam update :)
/// Also sets object's StartLocation and ImpactLocation.
/// </summary>
/// <returns>Hit result Array</returns>
TArray<FHitResult> ALaserWeapon::GetViewTarget() {

	World = this->GetWorld();

	FVector Location = PlayerCharacter->GetActorLocation();
	Location += WeaponOffset + PlayerCharacter->GetActorForwardVector() * 50;

	FVector TraceStart = UGameplayStatics::GetPlayerCameraManager(World, 0)->GetCameraLocation();
	FVector TraceEnd = TraceStart + PlayerCharacter->GetActorForwardVector() * 2500;

	//ImpactLocation gets overwritten a bunch depending on if it hits or not. It's used for the particle effects, so you want it to be at the final hit.
	StartLocation = Location;
	ImpactLocation = TraceEnd;

	ECollisionChannel Channel = ECC_GameTraceChannel2; //AimTrace.
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerCharacter);

	TArray<FHitResult> Hit;

	World->LineTraceMultiByChannel(Hit, TraceStart, TraceEnd, Channel, QueryParams);

	return Hit;
}


void ALaserWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bSNiagaraExist == true) {
		TArray<FHitResult> Hit = GetViewTarget();
		if (Hit.IsValidIndex(0)) {
			ImpactLocation = Hit[0].Location;

		}
		else {
			FVector TraceStart = UGameplayStatics::GetPlayerCameraManager(World, 0)->GetCameraLocation();
			FVector TraceEnd = TraceStart + UKismetMathLibrary::GetForwardVector(UGameplayStatics::GetPlayerCameraManager(World, 0)->GetCameraRotation()) * 10000;
			ImpactLocation = TraceEnd;
		}
		StartLocation = PlayerCharacter->GetActorLocation() + WeaponOffset;
		UpdateNiagaraBPConstant();
	}
}