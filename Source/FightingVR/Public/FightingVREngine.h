// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVREngine.generated.h"

UCLASS()
class FIGHTINGVR_API UFightingVREngine : public UGameEngine
{
	GENERATED_UCLASS_BODY()

	/* Hook up specific callbacks */
	virtual void Init(IEngineLoop* InEngineLoop);

public:

	/**
	 * 	All regular engine handling, plus update FightingVRKing state appropriately.
	 */
	virtual void HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) override;
};

