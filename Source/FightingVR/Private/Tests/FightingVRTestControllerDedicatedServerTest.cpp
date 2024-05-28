// Copyright Epic Games, Inc.All Rights Reserved.
#include "FightingVRTestControllerDedicatedServerTest.h"
#include "FightingVRSession.h"

void UFightingVRTestControllerDedicatedServerTest::OnTick(float TimeDelta)
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