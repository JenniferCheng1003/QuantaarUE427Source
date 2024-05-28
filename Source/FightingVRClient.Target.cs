// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class FightingVRClientTarget : TargetRules
{
	public FightingVRClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Client;
		bUsesSteam = true;		

        ExtraModuleNames.Add("FightingVR");
    }
}
