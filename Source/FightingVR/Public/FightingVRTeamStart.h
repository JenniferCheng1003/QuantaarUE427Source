// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRTeamStart.generated.h"

UCLASS()
class AFightingVRTeamStart : public APlayerStart
{
	GENERATED_UCLASS_BODY()

	/** Which team can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	int32 SpawnTeam;

	/** Whether players can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	uint32 bNotForPlayers:1;

	/** Whether bots can start at this point */
	UPROPERTY(EditInstanceOnly, Category=Team)
	uint32 bNotForBots:1;
};
