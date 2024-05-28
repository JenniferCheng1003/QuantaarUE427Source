// Copyright Epic Games, Inc.All Rights Reserved.
#include "FightingVRTestControllerBasicDedicatedServerTest.h"
#include "FightingVRInstance.h"

void UFightingVRTestControllerBasicDedicatedServerTest::OnTick(float TimeDelta)
{
	if (GetTimeInCurrentState() > 300)
	{
		UE_LOG(LogGauntlet, Error, TEXT("Failing boot test after 300 secs!"));
		EndTest(-1);
	}
}

void UFightingVRTestControllerBasicDedicatedServerTest::OnPostMapChange(UWorld* World)
{
	if (IsInGame())
	{
		EndTest(0);
	}
}
