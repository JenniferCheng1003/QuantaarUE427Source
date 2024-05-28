// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerBase.h"
#include "FightingVROptionsWidgetStyle.generated.h"

/**
 * Represents the appearance of an FFightingVROptions
 */
USTRUCT()
struct FFightingVROptionsStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FFightingVROptionsStyle();
	virtual ~FFightingVROptionsStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FFightingVROptionsStyle& GetDefault();

	/**
	 * The sound the options should play when changes are accepted
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound AcceptChangesSound;
	FFightingVROptionsStyle& SetAcceptChangesSound(const FSlateSound& InAcceptChangesSound) { AcceptChangesSound = InAcceptChangesSound; return *this; }

	/**
	 * The sound the options should play when changes are discarded
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound DiscardChangesSound;
	FFightingVROptionsStyle& SetDiscardChangesSound(const FSlateSound& InDiscardChangesSound) { DiscardChangesSound = InDiscardChangesSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UFightingVROptionsWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FFightingVROptionsStyle OptionsStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &OptionsStyle );
	}
};
