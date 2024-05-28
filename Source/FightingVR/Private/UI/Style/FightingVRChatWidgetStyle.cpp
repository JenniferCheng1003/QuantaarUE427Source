// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRChatWidgetStyle.h"
#include "FightingVR.h"

FFightingVRChatStyle::FFightingVRChatStyle()
{
}

FFightingVRChatStyle::~FFightingVRChatStyle()
{
}

const FName FFightingVRChatStyle::TypeName(TEXT("FFightingVRChatStyle"));

const FFightingVRChatStyle& FFightingVRChatStyle::GetDefault()
{
	static FFightingVRChatStyle Default;
	return Default;
}

void FFightingVRChatStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	TextEntryStyle.GetResources(OutBrushes);

	OutBrushes.Add(&BackingBrush);
}


UFightingVRChatWidgetStyle::UFightingVRChatWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
