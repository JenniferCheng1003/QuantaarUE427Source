// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRTypes.h"
#include "FightingVRViewportClient.generated.h"

class SFightingVRConfirmationDialog;

struct FFightingVRLoadingScreenBrush : public FSlateDynamicImageBrush, public FGCObject
{
	FFightingVRLoadingScreenBrush( const FName InTextureName, const FVector2D& InImageSize )
		: FSlateDynamicImageBrush( InTextureName, InImageSize )
	{
		SetResourceObject(LoadObject<UObject>( nullptr, *InTextureName.ToString() ));
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector)
	{
		FSlateBrush::AddReferencedObjects(Collector);
	}
};

class SFightingVRLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFightingVRLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EVisibility GetLoadIndicatorVisibility() const
	{
		return EVisibility::Visible;
	}

	/** loading screen image brush */
	TSharedPtr<FSlateDynamicImageBrush> LoadingScreenBrush;
};

UCLASS(Within=Engine, transient, config=Engine)
class UFightingVRViewportClient : public UGameViewportClient
{
	GENERATED_UCLASS_BODY()

public:

 	// start UGameViewportClient interface
 	void NotifyPlayerAdded( int32 PlayerIndex, ULocalPlayer* AddedPlayer ) override;
	void AddViewportWidgetContent( TSharedRef<class SWidget> ViewportContent, const int32 ZOrder = 0 ) override;
	void RemoveViewportWidgetContent( TSharedRef<class SWidget> ViewportContent ) override;

	void ShowDialog(TWeakObjectPtr<ULocalPlayer> PlayerOwner, EFightingVRDialogType::Type DialogType, const FText& Message, const FText& Confirm, const FText& Cancel, const FOnClicked& OnConfirm, const FOnClicked& OnCancel);
	void HideDialog();

	void ShowLoadingScreen();
	void HideLoadingScreen();

	bool IsShowingDialog() const { return DialogWidget.IsValid(); }

	EFightingVRDialogType::Type GetDialogType() const;
	TWeakObjectPtr<ULocalPlayer> GetDialogOwner() const;

	TSharedPtr<SFightingVRConfirmationDialog> GetDialogWidget() { return DialogWidget; }

	//FTicker Funcs
	virtual void Tick(float DeltaSeconds) override;	

	virtual	void BeginDestroy() override;
	virtual void DetachViewportClient() override;
	void ReleaseSlateResources();

#if WITH_EDITOR
	virtual void DrawTransition(class UCanvas* Canvas) override;
#endif //WITH_EDITOR
	// end UGameViewportClient interface

protected:
	void HideExistingWidgets();
	void ShowExistingWidgets();

	/** List of viewport content that the viewport is tracking */
	TArray<TSharedRef<class SWidget>>				ViewportContentStack;

	TArray<TSharedRef<class SWidget>>				HiddenViewportContentStack;

	TSharedPtr<class SWidget>						OldFocusWidget;

	/** Dialog widget to show temporary messages ("Controller disconnected", "Parental Controls don't allow you to play online", etc) */
	TSharedPtr<SFightingVRConfirmationDialog>			DialogWidget;

	TSharedPtr<SFightingVRLoadingScreen>				LoadingScreenWidget;
};