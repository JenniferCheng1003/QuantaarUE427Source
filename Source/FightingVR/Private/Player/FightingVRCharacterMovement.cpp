// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRCharacterMovement.h"
#include "FightingVR.h"


//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UFightingVRCharacterMovement::UFightingVRCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


float UFightingVRCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AFightingVRCharacter* FightingVRCharacterOwner = Cast<AFightingVRCharacter>(PawnOwner);
	if (FightingVRCharacterOwner)
	{
		if (FightingVRCharacterOwner->IsTargeting())
		{
			MaxSpeed *= FightingVRCharacterOwner->GetTargetingSpeedModifier();
		}
		if (FightingVRCharacterOwner->IsRunning())
		{
			MaxSpeed *= FightingVRCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}
