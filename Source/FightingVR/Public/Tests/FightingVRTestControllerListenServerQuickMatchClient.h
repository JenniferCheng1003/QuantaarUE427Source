// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "FightingVRTestControllerBase.h"
#include "FightingVRTestControllerListenServerQuickMatchClient.generated.h"

UCLASS()
class UFightingVRTestControllerListenServerQuickMatchClient : public UFightingVRTestControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};