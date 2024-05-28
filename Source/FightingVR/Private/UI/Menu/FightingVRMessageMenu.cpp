// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRMessageMenu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "SFightingVRConfirmationDialog.h"
#include "FightingVRViewportClient.h"
#include "FightingVRInstance.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

void FFightingVRMessageMenu::Construct(TWeakObjectPtr<UFightingVRInstance> InGameInstance, TWeakObjectPtr<ULocalPlayer> InPlayerOwner, const FText& Message, const FText& OKButtonText, const FText& CancelButtonText, const FName& InPendingNextState)
{
	GameInstance			= InGameInstance;
	PlayerOwner				= InPlayerOwner;
	PendingNextState		= InPendingNextState;

	if ( ensure( GameInstance.IsValid() ) )
	{
		UFightingVRViewportClient* FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

		if ( FightingVRViewport )
		{
			// Hide the previous dialog
			FightingVRViewport->HideDialog();

			// Show the new one
			FightingVRViewport->ShowDialog( 
				PlayerOwner,
				EFightingVRDialogType::Generic,
				Message, 
				OKButtonText, 
				CancelButtonText, 
				FOnClicked::CreateRaw(this, &FFightingVRMessageMenu::OnClickedOK),
				FOnClicked::CreateRaw(this, &FFightingVRMessageMenu::OnClickedCancel)
			);
		}
	}
}

void FFightingVRMessageMenu::RemoveFromGameViewport()
{
	if ( ensure( GameInstance.IsValid() ) )
	{
		UFightingVRViewportClient * FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

		if ( FightingVRViewport )
		{
			// Hide the previous dialog
			FightingVRViewport->HideDialog();
		}
	}
}

void FFightingVRMessageMenu::HideDialogAndGotoNextState()
{
	RemoveFromGameViewport();

	if ( ensure( GameInstance.IsValid() ) )
	{
		GameInstance->GotoState( PendingNextState );
	}
};

FReply FFightingVRMessageMenu::OnClickedOK()
{
	OKButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

FReply FFightingVRMessageMenu::OnClickedCancel()
{
	CancelButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

void FFightingVRMessageMenu::SetOKClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	OKButtonDelegate = InButtonDelegate;
}

void FFightingVRMessageMenu::SetCancelClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	CancelButtonDelegate = InButtonDelegate;
}


#undef LOCTEXT_NAMESPACE
