// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRPickup.h"
#include "FightingVRPickup_Ammo.generated.h"

class AFightingVRCharacter;
class AFightingVRWeapon;

// A pickup object that replenishes ammunition for a weapon
UCLASS(Abstract, Blueprintable)
class AFightingVRPickup_Ammo : public AFightingVRPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(AFightingVRCharacter* TestPawn) const override;

	bool IsForWeapon(UClass* WeaponClass);

protected:

	/** how much ammo does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 AmmoClips;

	/** which weapon gets ammo? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	TSubclassOf<AFightingVRWeapon> WeaponType;

	/** give pickup */
	virtual void GivePickupTo(AFightingVRCharacter* Pawn) override;
};
