// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRMenuWidgetStyle.h"
#include "FightingVR.h"

FFightingVRMenuStyle::FFightingVRMenuStyle()
{
}

FFightingVRMenuStyle::~FFightingVRMenuStyle()
{
}

const FName FFightingVRMenuStyle::TypeName(TEXT("FFightingVRMenuStyle"));

const FFightingVRMenuStyle& FFightingVRMenuStyle::GetDefault()
{
	static FFightingVRMenuStyle Default;
	return Default;
}

void FFightingVRMenuStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&HeaderBackgroundBrush);
	OutBrushes.Add(&LeftBackgroundBrush);
	OutBrushes.Add(&RightBackgroundBrush);
}


UFightingVRMenuWidgetStyle::UFightingVRMenuWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
