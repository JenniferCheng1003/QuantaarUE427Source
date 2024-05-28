// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRPlayerController_Menu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"


AFightingVRPlayerController_Menu::AFightingVRPlayerController_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AFightingVRPlayerController_Menu::PostInitializeComponents() 
{
	Super::PostInitializeComponents();

	FFightingVRStyle::Initialize();
}
