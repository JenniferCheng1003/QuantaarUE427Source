// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class FightingVR : ModuleRules
{
	public FightingVR(ReadOnlyTargetRules Target) : base(Target)
	{
		
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivatePCHHeaderFile = "Public/FightingVR.h";
		
		PublicDefinitions.Add("HOST_ONLINE_GAMEMODE_ENABLED=" + HostOnlineGameEnabled);
		PublicDefinitions.Add("JOIN_ONLINE_GAME_ENABLED=" + JoinOnlineGameEnabled);
		PublicDefinitions.Add("INVITE_ONLINE_GAME_ENABLED=" + InviteOnlineGameEnabled);
		PublicDefinitions.Add("ONLINE_STORE_ENABLED=" + OnlineStoreEnabled);

		PrivateIncludePaths.AddRange(
			new string[] { 
				"FightingVR/Private",
				"FightingVR/Private/UI",
				"FightingVR/Private/UI/Menu",
				"FightingVR/Private/UI/Style",
				"FightingVR/Private/UI/Widgets"
            }
		);

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore", 
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"AssetRegistry",
				"NavigationSystem",
				"AIModule",
				"GameplayTasks",
				"Gauntlet",
				"DeveloperSettings"
            }
		);

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"VivoxCore",
				"InputCore",
				"Slate",
				"SlateCore",
				"Json",
				"HTTP",
				"ApplicationCore",
				"ReplicationGraph",
				"PakFile",
				"RHI",
				"PhysicsCore"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"OnlineSubsystemNull",
				"NetworkReplayStreaming",
				"NullNetworkReplayStreaming",
				"HttpNetworkReplayStreaming",
				"LocalFileNetworkReplayStreaming"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"NetworkReplayStreaming"
			}
		);

		if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
        {
            PrivateDependencyModuleNames.Add("GameplayDebugger");
            PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
        }
		else
		{
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}
		
	}
	
	protected virtual int HostOnlineGameEnabled 
	{ 
		get 
		{ 
			return 1; 
		} 
	}

	protected virtual int JoinOnlineGameEnabled
    {
        get
        {
			return 1;
        }
    }

	protected virtual int InviteOnlineGameEnabled
    {
		get
        {
			return 1;
        }
    }

	protected virtual int OnlineStoreEnabled
	{
		get
		{
			return 1;
		}
	}
	
}
