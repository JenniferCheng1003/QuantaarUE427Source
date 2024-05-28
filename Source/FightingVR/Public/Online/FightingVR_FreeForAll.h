// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVR_FreeForAll.generated.h"

class AFightingVRPlayerState;

UCLASS()
class AFightingVR_FreeForAll : public AFightingVRMode
{
	GENERATED_UCLASS_BODY()

protected:

	/** best player */
	UPROPERTY(transient)
	AFightingVRPlayerState* WinnerPlayerState;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AFightingVRPlayerState* PlayerState) const override;
};
