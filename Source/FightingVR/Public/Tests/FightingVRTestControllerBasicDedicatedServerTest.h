// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "Tests/FightingVRTestControllerBase.h"
#include "FightingVRTestControllerBasicDedicatedServerTest.generated.h"

UCLASS()
class UFightingVRTestControllerBasicDedicatedServerTest : public UFightingVRTestControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;

public:
	virtual void OnPostMapChange(UWorld* World) override;
};