// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRMenuSoundsWidgetStyle.h"
#include "FightingVR.h"

FFightingVRMenuSoundsStyle::FFightingVRMenuSoundsStyle()
{
}

FFightingVRMenuSoundsStyle::~FFightingVRMenuSoundsStyle()
{
}

const FName FFightingVRMenuSoundsStyle::TypeName(TEXT("FFightingVRMenuSoundsStyle"));

const FFightingVRMenuSoundsStyle& FFightingVRMenuSoundsStyle::GetDefault()
{
	static FFightingVRMenuSoundsStyle Default;
	return Default;
}

void FFightingVRMenuSoundsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


UFightingVRMenuSoundsWidgetStyle::UFightingVRMenuSoundsWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
