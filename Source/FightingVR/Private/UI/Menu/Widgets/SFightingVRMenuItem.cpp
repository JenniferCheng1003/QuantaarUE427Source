// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFightingVRMenuItem.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "FightingVRMenuItemWidgetStyle.h"

void SFightingVRMenuItem::Construct(const FArguments& InArgs)
{
	ItemStyle = &FFightingVRStyle::Get().GetWidgetStyle<FFightingVRMenuItemStyle>("DefaultFightingVRMenuItemStyle");

	PlayerOwner = InArgs._PlayerOwner;
	Text = InArgs._Text;
	OptionText = InArgs._OptionText;
	OnClicked = InArgs._OnClicked;
	OnArrowPressed = InArgs._OnArrowPressed;
	bIsMultichoice = InArgs._bIsMultichoice;
	bIsActiveMenuItem = false;
	LeftArrowVisible = EVisibility::Collapsed;
	RightArrowVisible = EVisibility::Collapsed;
	//if attribute is set, use its value, otherwise uses default
	InactiveTextAlpha = InArgs._InactiveTextAlpha.Get(1.0f);

	const float ArrowMargin = 3.0f;
	ItemMargin = 10.0f;
	TextColor = FLinearColor(FColor(155,164,182));

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			.WidthOverride(374.0f)
			.HeightOverride(23.0f)
			[
				SNew(SImage)
				.ColorAndOpacity(this,&SFightingVRMenuItem::GetButtonBgColor)
				.Image(&ItemStyle->BackgroundBrush)
			]
		]
		+SOverlay::Slot()
		.HAlign(bIsMultichoice ? HAlign_Left : HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(ItemMargin,0,0,0))
		[
			SAssignNew(TextWidget, STextBlock)
			.TextStyle(FFightingVRStyle::Get(), "FightingVR.MenuTextStyle")
			.ColorAndOpacity(this,&SFightingVRMenuItem::GetButtonTextColor)
			.ShadowColorAndOpacity(this, &SFightingVRMenuItem::GetButtonTextShadowColor)
			.Text(Text)
		]
		+SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.Padding(FMargin(0,0,ArrowMargin,0))
				.Visibility(this,&SFightingVRMenuItem::GetLeftArrowVisibility)
				.OnMouseButtonDown(this,&SFightingVRMenuItem::OnLeftArrowDown)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(&ItemStyle->LeftArrowImage)
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(TAttribute<FMargin>(this, &SFightingVRMenuItem::GetOptionPadding))
			[
				SNew(STextBlock)
				.TextStyle(FFightingVRStyle::Get(), "FightingVR.MenuTextStyle")
				.Visibility(bIsMultichoice ? EVisibility:: Visible : EVisibility::Collapsed )
				.ColorAndOpacity(this,&SFightingVRMenuItem::GetButtonTextColor)
				.ShadowColorAndOpacity(this, &SFightingVRMenuItem::GetButtonTextShadowColor)
				.Text(OptionText)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				.Padding(FMargin(ArrowMargin,0,ItemMargin,0))
				.Visibility(this,&SFightingVRMenuItem::GetRightArrowVisibility)
				.OnMouseButtonDown(this,&SFightingVRMenuItem::OnRightArrowDown)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(&ItemStyle->RightArrowImage)
					]
				]
			]
		]
		
	];
}

FReply SFightingVRMenuItem::OnRightArrowDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = FReply::Unhandled();
	const int32 MoveRight = 1;
	if (OnArrowPressed.IsBound() && bIsActiveMenuItem)
	{
		OnArrowPressed.Execute(MoveRight);
		Result = FReply::Handled();
	}
	return Result;
}

FReply SFightingVRMenuItem::OnLeftArrowDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Result = FReply::Unhandled();
	const int32 MoveLeft = -1;
	if (OnArrowPressed.IsBound() && bIsActiveMenuItem)
	{
		OnArrowPressed.Execute(MoveLeft);
		Result = FReply::Handled();
	}
	return Result;
}

EVisibility SFightingVRMenuItem::GetLeftArrowVisibility() const
{
	return LeftArrowVisible;
}

EVisibility SFightingVRMenuItem::GetRightArrowVisibility() const
{
	return RightArrowVisible;
}

FMargin SFightingVRMenuItem::GetOptionPadding() const
{
	return RightArrowVisible == EVisibility::Visible ? FMargin(0) : FMargin(0,0,ItemMargin,0);
}

FSlateColor SFightingVRMenuItem::GetButtonTextColor() const
{
	FLinearColor Result;
	if (bIsActiveMenuItem)
	{
		Result = TextColor;
	}
	else
	{
		Result = FLinearColor(TextColor.R, TextColor.G, TextColor.B, InactiveTextAlpha);
	}
	return Result;
}

FLinearColor SFightingVRMenuItem::GetButtonTextShadowColor() const
{
	FLinearColor Result;
	if (bIsActiveMenuItem)
	{
		Result = FLinearColor(0,0,0,1);
	}
	else
	{
		Result = FLinearColor(0,0,0, InactiveTextAlpha);
	}
	return Result;
}


FSlateColor SFightingVRMenuItem::GetButtonBgColor() const
{
	const float MinAlpha = 0.1f;
	const float MaxAlpha = 1.f;
	const float AnimSpeedModifier = 1.5f;
	
	float AnimPercent = 0.f;
	ULocalPlayer* const Player = PlayerOwner.Get();
	if (Player)
	{
		// @fixme, need a world get delta time?
		UWorld* const World = Player->GetWorld();
		if (World)
		{
			const float GameTime = World->GetRealTimeSeconds();
			AnimPercent = FMath::Abs(FMath::Sin(GameTime*AnimSpeedModifier));
		}
	}

	const float BgAlpha = bIsActiveMenuItem ? FMath::Lerp(MinAlpha, MaxAlpha, AnimPercent) : 0.f;
	return FLinearColor(1.f, 1.f, 1.f, BgAlpha);
}

FReply SFightingVRMenuItem::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//execute our "OnClicked" delegate, if we have one
	if(OnClicked.IsBound() == true)
	{
		return OnClicked.Execute();
	}

	return FReply::Handled();
}


FReply SFightingVRMenuItem::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

void SFightingVRMenuItem::SetMenuItemActive(bool bIsMenuItemActive)
{
	this->bIsActiveMenuItem = bIsMenuItemActive;
}

void SFightingVRMenuItem::UpdateItemText(const FText& UpdatedText)
{
	Text = UpdatedText;
	if (TextWidget.IsValid())
	{
		TextWidget->SetText(Text);
	}
}
