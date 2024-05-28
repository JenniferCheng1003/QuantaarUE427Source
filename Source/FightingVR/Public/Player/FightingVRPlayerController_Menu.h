// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRTypes.h"
#include "FightingVRPlayerController_Menu.generated.h"

UCLASS()
class AFightingVRPlayerController_Menu : public APlayerController
{
	GENERATED_UCLASS_BODY()

	/** After game is initialized */
	virtual void PostInitializeComponents() override;
};

