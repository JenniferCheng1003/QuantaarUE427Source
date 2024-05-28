// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVROptionsWidgetStyle.h"
#include "FightingVR.h"

FFightingVROptionsStyle::FFightingVROptionsStyle()
{
}

FFightingVROptionsStyle::~FFightingVROptionsStyle()
{
}

const FName FFightingVROptionsStyle::TypeName(TEXT("FFightingVROptionsStyle"));

const FFightingVROptionsStyle& FFightingVROptionsStyle::GetDefault()
{
	static FFightingVROptionsStyle Default;
	return Default;
}

void FFightingVROptionsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


UFightingVROptionsWidgetStyle::UFightingVROptionsWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
