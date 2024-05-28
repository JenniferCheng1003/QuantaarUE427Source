// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "FightingVRTestControllerBase.h"
#include "FightingVRTestControllerListenServerClient.generated.h"

UCLASS()
class UFightingVRTestControllerListenServerClient : public UFightingVRTestControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};