// Copyright Epic Games, Inc.All Rights Reserved.
#include "FightingVRTestControllerListenServerClient.h"
#include "FightingVRSession.h"

void UFightingVRTestControllerListenServerClient::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);

	if (bIsLoggedIn && !bIsSearchingForGame && !bFoundGame)
	{
		StartSearchingForGame();
	}

	if (bIsSearchingForGame && !bFoundGame)
	{
		UpdateSearchStatus();
	}
}