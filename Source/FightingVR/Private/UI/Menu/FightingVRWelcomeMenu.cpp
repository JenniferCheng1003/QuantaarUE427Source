// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRWelcomeMenu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "SFightingVRConfirmationDialog.h"
#include "FightingVRViewportClient.h"
#include "FightingVRInstance.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

class SFightingVRWelcomeMenuWidget : public SCompoundWidget
{
	/** The menu that owns this widget. */
	FFightingVRWelcomeMenu* MenuOwner;

	/** Animate the text so that the screen isn't static, for console cert requirements. */
	FCurveSequence TextAnimation;

	/** The actual curve that animates the text. */
	FCurveHandle TextColorCurve;

	TSharedPtr<SRichTextBlock> PressPlayText;

	/* On the first tick ensure we have set the keyboard focus */
	bool bFirstTick = true;

	SLATE_BEGIN_ARGS( SFightingVRWelcomeMenuWidget )
	{}

	SLATE_ARGUMENT(FFightingVRWelcomeMenu*, MenuOwner)

	SLATE_END_ARGS()

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	UWorld* GetWorld() const
	{
		if (MenuOwner && MenuOwner->GetGameInstance().IsValid())
		{
			return MenuOwner->GetGameInstance()->GetWorld();
		}

		return nullptr;
	}

	void Construct( const FArguments& InArgs )
	{
		MenuOwner = InArgs._MenuOwner;
		
		TextAnimation = FCurveSequence();
		const float AnimDuration = 1.5f;
		TextColorCurve = TextAnimation.AddCurve(0, AnimDuration, ECurveEaseFunction::QuadInOut);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(30.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[ 
				SAssignNew( PressPlayText, SRichTextBlock )
#if PLATFORM_PS4
				.Text( LOCTEXT("PressStartPS4", "PRESS CROSS BUTTON TO PLAY" ) )
#elif PLATFORM_SWITCH
				.Text(LOCTEXT("PressStartSwitch", "PRESS <img src=\"FightingVR.Switch.Right\"/> TO PLAY"))
#elif FIGHTINGVR_XBOX_STRINGS
				.Text( LOCTEXT("PressStartXboxOne", "PRESS A TO PLAY" ) )
#else
				.Text( LOCTEXT("PressStartPC", "PRESS ENTER TO PLAY" ) )
#endif
				.TextStyle( FFightingVRStyle::Get(), "FightingVR.WelcomeScreen.WelcomeTextStyle" )
				.DecoratorStyleSet(&FFightingVRStyle::Get())
				+ SRichTextBlock::ImageDecorator()
			]
		];
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		// During construction we may miss out on setting focus of the keyboard due to the GameViewport not having its Parent pointer
		// setup. If this happens we will be unable to set the keyboard focus in AddToGameViewport()
		if (bFirstTick)
		{
			bFirstTick = false;
			FSlateApplication::Get().SetKeyboardFocus(this->AsShared());
		}

		if(!TextAnimation.IsPlaying())
		{
			if(TextAnimation.IsAtEnd())
			{
				TextAnimation.PlayReverse(this->AsShared());
			}
			else
			{
				TextAnimation.Play(this->AsShared());
			}
		}

		PressPlayText->SetRenderOpacity(FMath::Lerp(0.5f, 1.0f, TextColorCurve.GetLerp()));
	}

	virtual FReply OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Enter)
		{
			TSharedPtr<const FUniqueNetId> UserId;
			const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
			if (OnlineSub)
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
				}
			}
			MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
		}
		else if (!MenuOwner->GetControlsLocked() && Key == EKeys::Virtual_Accept)
		{
			bool bSkipToMainMenu = true;

			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
						const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

						const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(InKeyEvent.GetUserIndex());
						if (LoginStatus == ELoginStatus::NotLoggedIn || !bIsLicensed)
						{
							// Show the account picker.
							const IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
							if (ExternalUI.IsValid())
							{
								ExternalUI->ShowLoginUI(InKeyEvent.GetUserIndex(), false, true, FOnLoginUIClosedDelegate::CreateSP(MenuOwner, &FFightingVRWelcomeMenu::HandleLoginUIClosed));
								bSkipToMainMenu = false;
							}
						}
					}
				}
			}

			if (bSkipToMainMenu)
			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
						// If we couldn't show the external login UI for any reason, or if the user is
						// already logged in, just advance to the main menu immediately.
						MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
					}
				}
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override
	{
		bFirstTick = true;
	}

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis( this ), EFocusCause::SetDirectly, true);
	}
};

void FFightingVRWelcomeMenu::Construct( TWeakObjectPtr< UFightingVRInstance > InGameInstance )
{
	bControlsLocked = false;
	GameInstance = InGameInstance;
	PendingControllerIndex = -1;

	MenuWidget = SNew( SFightingVRWelcomeMenuWidget )
		.MenuOwner(this);	
}

void FFightingVRWelcomeMenu::AddToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef());
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

void FFightingVRWelcomeMenu::RemoveFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
	}
}

void FFightingVRWelcomeMenu::HandleLoginUIClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const FOnlineError& Error)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	UFightingVRViewportClient* FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// If they don't currently have a license, let them know, but don't let them proceed
	if (!bIsLicensed && FightingVRViewport != NULL)
	{
		const FText StopReason	= NSLOCTEXT( "ProfileMessages", "NeedLicense", "The signed in users do not have a license for this game. Please purchase FightingVR from the Xbox Marketplace or sign in a user with a valid license." );
		const FText OKButton	= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		FightingVRViewport->ShowDialog( 
			nullptr,
			EFightingVRDialogType::Generic,
			StopReason,
			OKButton,
			FText::GetEmpty(),
			FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnConfirmGeneric),
			FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnConfirmGeneric)
			);
		return;
	}

	PendingControllerIndex = ControllerIndex;

	if (UniqueId.IsValid())
	{
		// Next step, check privileges
		const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GameInstance->GetWorld());
		if (OnlineSub)
		{
			const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				IdentityInterface->GetUserPrivilege(*UniqueId, EUserPrivileges::CanPlay, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FFightingVRWelcomeMenu::OnUserCanPlay));
			}
		}
	}
	else
	{
		// Show a warning that your progress won't be saved if you continue without logging in. 
		if (FightingVRViewport != NULL)
		{
			FightingVRViewport->ShowDialog( 
				nullptr,
				EFightingVRDialogType::Generic,
				NSLOCTEXT("ProfileMessages", "ProgressWillNotBeSaved", "If you continue without signing in, your progress will not be saved."),
#if FIGHTINGVR_XBOX_STRINGS
				NSLOCTEXT("DialogButtons", "AContinue", "A - Continue"),
				NSLOCTEXT("DialogButtons", "BBack", "B - Back"),
#else
				NSLOCTEXT("DialogButtons", "EnterContinue", "Enter - Continue"),
				NSLOCTEXT("DialogButtons", "EscBack", "Esc - Back"),
#endif
				FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnContinueWithoutSavingConfirm),
				FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

void FFightingVRWelcomeMenu::SetControllerAndAdvanceToMainMenu(const int ControllerIndex)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	ULocalPlayer * NewPlayerOwner = GameInstance->GetFirstGamePlayer();

	if ( NewPlayerOwner != nullptr && ControllerIndex != -1 )
	{
		NewPlayerOwner->SetControllerId(ControllerIndex);
		NewPlayerOwner->SetCachedUniqueNetId(NewPlayerOwner->GetUniqueNetIdFromCachedControllerId().GetUniqueNetId());

		// tell gameinstance to transition to main menu
		GameInstance->GotoState(FightingVRInstanceState::MainMenu);
	}	
}

FReply FFightingVRWelcomeMenu::OnContinueWithoutSavingConfirm()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	UFightingVRViewportClient * FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

	if (FightingVRViewport != NULL)
	{
		FightingVRViewport->HideDialog();
	}

	SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	return FReply::Handled();
}

FReply FFightingVRWelcomeMenu::OnConfirmGeneric()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	UFightingVRViewportClient * FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

	if (FightingVRViewport != NULL)
	{
		FightingVRViewport->HideDialog();
	}

	return FReply::Handled();
}

void FFightingVRWelcomeMenu::OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	}
	else
	{
		UFightingVRViewportClient * FightingVRViewport = Cast<UFightingVRViewportClient>( GameInstance->GetGameViewportClient() );

		if ( FightingVRViewport != NULL )
		{
			const FText ReturnReason = NSLOCTEXT("PrivilegeFailures", "CannotPlayAgeRestriction", "You cannot play this game due to age restrictions.");
			const FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");

			FightingVRViewport->ShowDialog( 
				nullptr,
				EFightingVRDialogType::Generic,
				ReturnReason,
				OKButton,
				FText::GetEmpty(),
				FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnConfirmGeneric),
				FOnClicked::CreateRaw(this, &FFightingVRWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE
