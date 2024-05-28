// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRPickup.h"
#include "FightingVRPickup_Health.generated.h"

class AFightingVRCharacter;

// A pickup object that replenishes character health
UCLASS(Abstract, Blueprintable)
class AFightingVRPickup_Health : public AFightingVRPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(AFightingVRCharacter* TestPawn) const override;

protected:

	/** how much health does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 Health;

	/** give pickup */
	virtual void GivePickupTo(AFightingVRCharacter* Pawn) override;
};
