// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRScoreboardWidgetStyle.h"
#include "FightingVR.h"

FFightingVRScoreboardStyle::FFightingVRScoreboardStyle()
{
}

FFightingVRScoreboardStyle::~FFightingVRScoreboardStyle()
{
}

const FName FFightingVRScoreboardStyle::TypeName(TEXT("FFightingVRScoreboardStyle"));

const FFightingVRScoreboardStyle& FFightingVRScoreboardStyle::GetDefault()
{
	static FFightingVRScoreboardStyle Default;
	return Default;
}

void FFightingVRScoreboardStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&ItemBorderBrush);
}


UFightingVRScoreboardWidgetStyle::UFightingVRScoreboardWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
