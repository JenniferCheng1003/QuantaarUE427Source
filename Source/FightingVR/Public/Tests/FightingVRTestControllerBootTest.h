// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "GauntletTestControllerBootTest.h"
#include "FightingVRTestControllerBootTest.generated.h"

UCLASS()
class UFightingVRTestControllerBootTest : public UGauntletTestControllerBootTest
{
	GENERATED_BODY()

protected:

	// This test needs a delay as the test can be over before focus is returned to Gauntlet after launching the game on XBox.
	// This can cause the test to be over before Gauntlet can even know that it is running and will cause the test to fail.
	const double TestDelay = 20.0f;
	virtual bool IsBootProcessComplete() const override;
};