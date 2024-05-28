// Copyright Epic Games, Inc.All Rights Reserved.
#include "Tests/FightingVRTestControllerBootTest.h"
#include "FightingVRInstance.h"

bool UFightingVRTestControllerBootTest::IsBootProcessComplete() const
{
	static double StartTime = FPlatformTime::Seconds();
	const double TimeSinceStart = FPlatformTime::Seconds() - StartTime;

	if (TimeSinceStart >= TestDelay)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const UFightingVRInstance* GameInstance = Cast<UFightingVRInstance>(GetWorld()->GetGameInstance()))
			{
				if (GameInstance->GetCurrentState() == FightingVRInstanceState::WelcomeScreen ||
					GameInstance->GetCurrentState() == FightingVRInstanceState::MainMenu)
				{
					return true;
				}
			}
		}
	}

	return false;
}