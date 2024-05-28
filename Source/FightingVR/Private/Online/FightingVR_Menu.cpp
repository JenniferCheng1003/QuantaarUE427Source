// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVR_Menu.h"
#include "FightingVR.h"
#include "FightingVRMainMenu.h"
#include "FightingVRWelcomeMenu.h"
#include "FightingVRMessageMenu.h"
#include "FightingVRPlayerController_Menu.h"
#include "Online/FightingVRSession.h"

AFightingVR_Menu::AFightingVR_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = AFightingVRPlayerController_Menu::StaticClass();
}

void AFightingVR_Menu::RestartPlayer(class AController* NewPlayer)
{
	// don't restart
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AFightingVR_Menu::GetGameSessionClass() const
{
	return AFightingVRSession::StaticClass();
}
