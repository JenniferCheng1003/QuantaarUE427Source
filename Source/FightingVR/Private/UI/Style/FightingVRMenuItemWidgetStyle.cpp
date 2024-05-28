// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRMenuItemWidgetStyle.h"
#include "FightingVR.h"

FFightingVRMenuItemStyle::FFightingVRMenuItemStyle()
{
}

FFightingVRMenuItemStyle::~FFightingVRMenuItemStyle()
{
}

const FName FFightingVRMenuItemStyle::TypeName(TEXT("FFightingVRMenuItemStyle"));

const FFightingVRMenuItemStyle& FFightingVRMenuItemStyle::GetDefault()
{
	static FFightingVRMenuItemStyle Default;
	return Default;
}

void FFightingVRMenuItemStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&BackgroundBrush);
	OutBrushes.Add(&LeftArrowImage);
	OutBrushes.Add(&RightArrowImage);
}


UFightingVRMenuItemWidgetStyle::UFightingVRMenuItemWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
