// Copyright Epic Games, Inc.All Rights Reserved.
#include "FightingVRTestControllerListenServerQuickMatchClient.h"
#include "FightingVRSession.h"

void UFightingVRTestControllerListenServerQuickMatchClient::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);

	if (bIsLoggedIn && !bInQuickMatchSearch && !bFoundQuickMatchGame)
	{
		StartQuickMatch();
	}
}