// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "FightingVRTestControllerBase.h"
#include "SharedPointer.h"
#include "FightingVRTestControllerDedicatedServerTest.generated.h"

UCLASS()
class UFightingVRTestControllerDedicatedServerTest : public UFightingVRTestControllerBase
{
	GENERATED_BODY()

protected:
	virtual void OnTick(float TimeDelta) override;
};