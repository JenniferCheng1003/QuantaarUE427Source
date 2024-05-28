// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerBase.h"
#include "FightingVRMenuWidgetStyle.generated.h"

/**
 * Represents the appearance of an SFightingVRMenuWidget
 */
USTRUCT()
struct FFightingVRMenuStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FFightingVRMenuStyle();
	virtual ~FFightingVRMenuStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FFightingVRMenuStyle& GetDefault();

	/**
	 * The brush used for the header background
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HeaderBackgroundBrush;
	FFightingVRMenuStyle& SetHeaderBackgroundBrush(const FSlateBrush& InHeaderBackgroundBrush) { HeaderBackgroundBrush = InHeaderBackgroundBrush; return *this; }

	/**
	 * The brush used for the left side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LeftBackgroundBrush;
	FFightingVRMenuStyle& SetLeftBackgroundBrush(const FSlateBrush& InLeftBackgroundBrush) { LeftBackgroundBrush = InLeftBackgroundBrush; return *this; }

	/**
	 * The brush used for the right side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush RightBackgroundBrush;
	FFightingVRMenuStyle& SetRightBackgroundBrush(const FSlateBrush& InRightBackgroundBrush) { RightBackgroundBrush = InRightBackgroundBrush; return *this; }

	/**
	 * The sound that should play when entering a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuEnterSound;
	FFightingVRMenuStyle& SetMenuEnterSound(const FSlateSound& InMenuEnterSound) { MenuEnterSound = InMenuEnterSound; return *this; }

	/**
	 * The sound that should play when leaving a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuBackSound;
	FFightingVRMenuStyle& SetMenuBackSound(const FSlateSound& InMenuBackSound) { MenuBackSound = InMenuBackSound; return *this; }

	/**
	 * The sound that should play when changing an option
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound OptionChangeSound;
	FFightingVRMenuStyle& SetOptionChangeSound(const FSlateSound& InOptionChangeSound) { OptionChangeSound = InOptionChangeSound; return *this; }

	/**
	 * The sound that should play when changing the selected menu item
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuItemChangeSound;
	FFightingVRMenuStyle& SetMenuItemChangeSound(const FSlateSound& InMenuItemChangeSound) { MenuItemChangeSound = InMenuItemChangeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UFightingVRMenuWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FFightingVRMenuStyle MenuStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &MenuStyle );
	}
};
