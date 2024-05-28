// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class FightingVRGameTarget : TargetRules
{
    public FightingVRGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        bUsesSteam = true;

		ExtraModuleNames.Add("FightingVR");
    }
}
