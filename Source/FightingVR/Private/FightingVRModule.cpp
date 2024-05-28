// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVR.h"
#include "FightingVRDelegates.h"

#include "FightingVRMenuSoundsWidgetStyle.h"
#include "FightingVRMenuWidgetStyle.h"
#include "FightingVRMenuItemWidgetStyle.h"
#include "FightingVROptionsWidgetStyle.h"
#include "FightingVRScoreboardWidgetStyle.h"
#include "FightingVRChatWidgetStyle.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"



#include "UI/Style/FightingVRStyle.h"


class FFightingVRModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		InitializeFightingVRDelegates();
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		//Hot reload hack
		FSlateStyleRegistry::UnRegisterSlateStyle(FFightingVRStyle::GetStyleSetName());
		FFightingVRStyle::Initialize();
	}

	virtual void ShutdownModule() override
	{
		FFightingVRStyle::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFightingVRModule, FightingVR, "FightingVR");

DEFINE_LOG_CATEGORY(LogFightingVR)
DEFINE_LOG_CATEGORY(LogFightingVRWeapon)
