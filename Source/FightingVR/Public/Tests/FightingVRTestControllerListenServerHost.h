// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "FightingVRTestControllerBase.h"
#include "FightingVRTestControllerListenServerHost.generated.h"

UCLASS()
class UFightingVRTestControllerListenServerHost : public UFightingVRTestControllerBase
{
	GENERATED_BODY()

public:
	virtual void OnPostMapChange(UWorld* World) override {}

protected:
	virtual void OnUserCanPlayOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults) override;
};