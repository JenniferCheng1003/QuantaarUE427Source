// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRDemoSpectator.generated.h"

class SFightingVRDemoHUD;

UCLASS(config=Game)
class AFightingVRDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	/** FightingVR in-game menu */
	TSharedPtr<class FFightingVRDemoPlaybackMenu> FightingVRDemoPlaybackMenu;

	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;
	virtual void Destroyed() override;

	void OnToggleInGameMenu();
	void OnIncreasePlaybackSpeed();
	void OnDecreasePlaybackSpeed();

	int32 PlaybackSpeed;

private:
	TSharedPtr<SFightingVRDemoHUD> DemoHUD;
};

