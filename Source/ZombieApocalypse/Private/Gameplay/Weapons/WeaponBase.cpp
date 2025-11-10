// Copyright University of Inland Norway


#include "Gameplay/Weapons/WeaponBase.h"

// Sets default values
// Sets default values
AWeaponBase::AWeaponBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponBase::DoOnPurchase()
{
	//Set up player character 
	// S
	//Player = Cast<ACPP_Player>(UGameplayStatics::GetPlayerCharacter(World, 0));
	//Alternatively: Have Store/Player purchase function do this. It works the same but it's more adaptable probably.

}

void AWeaponBase::PrimaryFire()
{
	if (AWeaponBase::CanTrigger() == 0) {
		FTimerHandle Handle;
		//make it unable to shoot for a while ig.
		if (World) World->GetTimerManager().SetTimer(Handle, this, &AWeaponBase::ResetFireState, FireInterval, false);
	}
	//PlayerHUD->SetUIAmmo(Ammo, AmmoMax);

}

void AWeaponBase::SecondaryFire()
{
	if (AWeaponBase::CanTrigger() == 0) {
		FTimerHandle Handle;
		//make it unable to shoot for a while ig.
		if (World) World->GetTimerManager().SetTimer(Handle, this, &AWeaponBase::ResetFireState, FireInterval, false);
	}
	//PlayerHUD->SetUIAmmo(Ammo, AmmoMax);

}
/// <summary>
/// Calls timer function for reload. When I think about it, could probably have done this in the player, but writing player code is "not my job", so this is easier on the github.
/// </summary>
void AWeaponBase::Reload()
{
	FTimerHandle Handle2;
	bIsReloading = true;
	if (World) World->GetTimerManager().SetTimer(Handle2, this, &AWeaponBase::ResetReload, ReloadTime, false);
}


/// <summary>
///  It's useful to refer to the HUD in the weapon instead of doing it in the player. Mainly did this so the ammo would only update after pressing reload. When I think about it
/// I could probably have done that differently, but done is done ig.
/// There's probably a more direct way to do this
/// </summary>
/// <param name="Owner">Player character reference</param>
/// <param name="HUD">HUD reference</param>
/// <param name="bAttached">Is this weapon equipped or not</param>
void AWeaponBase::AttachWeapon(ACharacter* NOwner, UWeaponUIWidget* HUD)
{
	PlayerCharacter = NOwner;
	PlayerHUD = HUD;
	bIsAttached;
}

void AWeaponBase::ResetFireState() {
	bCanFire = true;
};

/// <summary>
/// Actually does the reloading math.
/// </summary>
void AWeaponBase::ResetReload() {
	if (AmmoMax > 0) {

		if (AmmoMax >= AmmoClip) {
			AmmoMax -= AmmoClip - Ammo;
			Ammo = AmmoClip;

		}
		else {
			Ammo = AmmoMax;
			AmmoMax = 0;
		}
	}

	//PlayerHUD->SetUIAmmo(Ammo, AmmoMax);
	bIsReloading = false;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Emerald, TEXT("Reloaded!"));
};


/// <summary>
/// Checks if it's already firing or has no ammo. Dazzit.
/// </summary>
/// <returns>Can it Trigger?</returns>
int AWeaponBase::CanTrigger() {
	if (bCanFire == false || bIsReloading == true) {
		return 0;
	}
	else if (Ammo <= 0) {
		if (NoAmmoSound != nullptr) {
			//UGameplayStatics::PlaySoundAtLocation(this, NoAmmoSound, Player->GetActorLocation());
		}
		return 0;
	}
	else return 1;
}