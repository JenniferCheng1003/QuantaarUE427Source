// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRIngameMenu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "FightingVRMenuSoundsWidgetStyle.h"
#include "Online.h"
#include "OnlineExternalUIInterface.h"
#include "FightingVRInstance.h"
#include "UI/FightingVRHUD.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

#if PLATFORM_SWITCH
#	define FRIENDS_SUPPORTED 0
#else
#	define FRIENDS_SUPPORTED 1
#endif

#if !defined(FRIENDS_IN_INGAME_MENU)
	#define FRIENDS_IN_INGAME_MENU 1
#endif

void FFightingVRIngameMenu::Construct(ULocalPlayer* _PlayerOwner)
{
	PlayerOwner = _PlayerOwner;
	bIsGameMenuUp = false;

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	
	//todo:  don't create ingame menus for remote players.
	const UFightingVRInstance* GameInstance = nullptr;
	if (PlayerOwner)
	{
		GameInstance = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
	}

	if (!GameMenuWidget.IsValid())
	{
		SAssignNew(GameMenuWidget, SFightingVRMenuWidget)
			.PlayerOwner(MakeWeakObjectPtr(PlayerOwner))
			.Cursor(EMouseCursor::Default)
			.IsGameMenu(true);			


		int32 const OwnerUserIndex = GetOwnerUserIndex();

		// setup the exit to main menu submenu.  We wanted a confirmation to avoid a potential TRC violation.
		// fixes TTP: 322267
		TSharedPtr<FFightingVRMenuItem> MainMenuRoot = FFightingVRMenuItem::CreateRoot();
		MainMenuItem = MenuHelper::AddMenuItem(MainMenuRoot,LOCTEXT("Main Menu", "MAIN MENU"));
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("No", "NO"), this, &FFightingVRIngameMenu::OnCancelExitToMain);
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("Yes", "YES"), this, &FFightingVRIngameMenu::OnConfirmExitToMain);		

		FightingVROptions = MakeShareable(new FFightingVROptions());
		FightingVROptions->Construct(PlayerOwner);
		FightingVROptions->TellInputAboutKeybindings();
		FightingVROptions->OnApplyChanges.BindSP(this, &FFightingVRIngameMenu::CloseSubMenu);

		MenuHelper::AddExistingMenuItem(RootMenuItem, FightingVROptions->CheatsItem.ToSharedRef());
		MenuHelper::AddExistingMenuItem(RootMenuItem, FightingVROptions->OptionsItem.ToSharedRef());

#if FRIENDS_SUPPORTED
		if (GameInstance && GameInstance->GetOnlineMode() == EOnlineMode::Online)
		{
#if !FRIENDS_IN_INGAME_MENU
			FightingVRFriends = MakeShareable(new FFightingVRFriends());
			FightingVRFriends->Construct(PlayerOwner, OwnerUserIndex);
			FightingVRFriends->TellInputAboutKeybindings();
			FightingVRFriends->OnApplyChanges.BindSP(this, &FFightingVRIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, FightingVRFriends->FriendsItem.ToSharedRef());

			FightingVRRecentlyMet = MakeShareable(new FFightingVRRecentlyMet());
			FightingVRRecentlyMet->Construct(PlayerOwner, OwnerUserIndex);
			FightingVRRecentlyMet->TellInputAboutKeybindings();
			FightingVRRecentlyMet->OnApplyChanges.BindSP(this, &FFightingVRIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, FightingVRRecentlyMet->RecentlyMetItem.ToSharedRef());
#endif		

#if FIGHTINGVR_CONSOLE_UI && INVITE_ONLINE_GAME_ENABLED			
			TSharedPtr<FFightingVRMenuItem> ShowInvitesItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Invite Players", "INVITE PLAYERS (via System UI)"));
			ShowInvitesItem->OnConfirmMenuItem.BindRaw(this, &FFightingVRIngameMenu::OnShowInviteUI);		
#endif
		}
#endif

		if (FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FFightingVRMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		MenuHelper::AddExistingMenuItem(RootMenuItem, MainMenuItem.ToSharedRef());
				
#if !FIGHTINGVR_CONSOLE_UI
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FFightingVRIngameMenu::OnUIQuit);
#endif

		GameMenuWidget->MainMenu = GameMenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		GameMenuWidget->OnMenuHidden.BindSP(this,&FFightingVRIngameMenu::DetachGameMenu);
		GameMenuWidget->OnToggleMenu.BindSP(this,&FFightingVRIngameMenu::ToggleGameMenu);
		GameMenuWidget->OnGoBack.BindSP(this, &FFightingVRIngameMenu::OnMenuGoBack);
	}
}

void FFightingVRIngameMenu::CloseSubMenu()
{
	GameMenuWidget->MenuGoBack();
}

void FFightingVRIngameMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (FightingVROptions.IsValid() && FightingVROptions->OptionsItem->SubMenu == Menu)
	{
		FightingVROptions->RevertChanges();
	}
}

bool FFightingVRIngameMenu::GetIsGameMenuUp() const
{
	return bIsGameMenuUp;
}

void FFightingVRIngameMenu::UpdateFriendsList()
{
	if (PlayerOwner)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		if (OnlineSub)
		{
			IOnlineFriendsPtr OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
			if (OnlineFriendsPtr.IsValid())
			{
				OnlineFriendsPtr->ReadFriendsList(GetOwnerUserIndex(), EFriendsLists::ToString(EFriendsLists::OnlinePlayers));
			}
		}
	}
}

void FFightingVRIngameMenu::DetachGameMenu()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GameMenuContainer.ToSharedRef());
	}
	bIsGameMenuUp = false;

	AFightingVRPlayerController* const PCOwner = PlayerOwner ? Cast<AFightingVRPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (PCOwner)
	{
		PCOwner->SetPause(false);

		// If the game is over enable the scoreboard
		AFightingVRHUD* const FightingVRHUD = PCOwner->GetFightingVRHUD();
		if( ( FightingVRHUD != NULL ) && ( FightingVRHUD->IsMatchOver() == true ) && ( PCOwner->IsPrimaryPlayer() == true ) )
		{
			FightingVRHUD->ShowScoreboard( true, true );
		}
	}
}

void FFightingVRIngameMenu::ToggleGameMenu()
{
	//Update the owner in case the menu was opened by another controller
	//UpdateMenuOwner();

	if (!GameMenuWidget.IsValid())
	{
		return;
	}

	// check for a valid user index.  could be invalid if the user signed out, in which case the 'please connect your control' ui should be up anyway.
	// in-game menu needs a valid userindex for many OSS calls.
	if (GetOwnerUserIndex() == -1)
	{
		UE_LOG(LogFightingVR, Log, TEXT("Trying to toggle in-game menu for invalid userid"));
		return;
	}

	if (bIsGameMenuUp && GameMenuWidget->CurrentMenu != RootMenuItem->SubMenu)
	{
		GameMenuWidget->MenuGoBack();
		return;
	}
	
	AFightingVRPlayerController* const PCOwner = PlayerOwner ? Cast<AFightingVRPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (!bIsGameMenuUp)
	{
		// Hide the scoreboard
		if (PCOwner)
		{
			AFightingVRHUD* const FightingVRHUD = PCOwner->GetFightingVRHUD();
			if( FightingVRHUD != NULL )
			{
				FightingVRHUD->ShowScoreboard( false );
			}
		}

		GEngine->GameViewport->AddViewportWidgetContent(
			SAssignNew(GameMenuContainer,SWeakWidget)
			.PossiblyNullContent(GameMenuWidget.ToSharedRef())
			);

		int32 const OwnerUserIndex = GetOwnerUserIndex();
		if(FightingVROptions.IsValid())
		{
			FightingVROptions->UpdateOptions();
		}
		if(FightingVRRecentlyMet.IsValid())
		{
			FightingVRRecentlyMet->UpdateRecentlyMet(OwnerUserIndex);
		}
		GameMenuWidget->BuildAndShowMenu();
		bIsGameMenuUp = true;

		if (PCOwner)
		{
			// Disable controls while paused
			PCOwner->SetCinematicMode(true, false, false, true, true);

			if (PCOwner->SetPause(true))
			{
				UFightingVRInstance* GameInstance = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
				GameInstance->SetPresenceForLocalPlayers(FString(TEXT("On Pause")), FVariantData(FString(TEXT("Paused"))));
			}
			
			FInputModeGameAndUI InputMode;
			PCOwner->SetInputMode(InputMode);
		}
	} 
	else
	{
		//Start hiding animation
		GameMenuWidget->HideMenu();
		if (PCOwner)
		{
			// Make sure viewport has focus
			FSlateApplication::Get().SetAllUserFocusToGameViewport();

			if (PCOwner->SetPause(false))
			{
				UFightingVRInstance* GameInstance = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
				GameInstance->SetPresenceForLocalPlayers(FString(TEXT("In Game")), FVariantData(FString(TEXT("InGame"))));
			}

			// Don't renable controls if the match is over
			AFightingVRHUD* const FightingVRHUD = PCOwner->GetFightingVRHUD();
			if( ( FightingVRHUD != NULL ) && ( FightingVRHUD->IsMatchOver() == false ) )
			{
				PCOwner->SetCinematicMode(false,false,false,true,true);

				FInputModeGameOnly InputMode;
				PCOwner->SetInputMode(InputMode);
			}
		}
	}
}

void FFightingVRIngameMenu::OnCancelExitToMain()
{
	CloseSubMenu();
}

void FFightingVRIngameMenu::OnConfirmExitToMain()
{
	UFightingVRInstance* const GameInstance = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->LabelPlayerAsQuitter(PlayerOwner);

		// tell game instance to go back to main menu state
		GameInstance->GotoState(FightingVRInstanceState::MainMenu);
	}
}

void FFightingVRIngameMenu::OnUIQuit()
{
	UFightingVRInstance* const GI = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
	if (GI)
	{
		GI->LabelPlayerAsQuitter(PlayerOwner);
	}

	GameMenuWidget->LockControls(true);
	GameMenuWidget->HideMenu();

	UWorld* const World = PlayerOwner ? PlayerOwner->GetWorld() : nullptr;
	if (World)
	{
		const FFightingVRMenuSoundsStyle& MenuSounds = FFightingVRStyle::Get().GetWidgetStyle<FFightingVRMenuSoundsStyle>("DefaultFightingVRMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.ExitGameSound, GetOwnerUserIndex(), this, &FFightingVRIngameMenu::Quit);
	}
}

void FFightingVRIngameMenu::Quit()
{
	APlayerController* const PCOwner = PlayerOwner ? PlayerOwner->PlayerController : nullptr;
	if (PCOwner)
	{
		PCOwner->ConsoleCommand("quit");
	}
}

void FFightingVRIngameMenu::OnShowInviteUI()
{
	if (PlayerOwner)
	{
		const IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(PlayerOwner->GetWorld());

		if (!ExternalUI.IsValid())
		{
			UE_LOG(LogFightingVR, Warning, TEXT("OnShowInviteUI: External UI interface is not supported on this platform."));
			return;
		}

		ExternalUI->ShowInviteUI(GetOwnerUserIndex());
	}
}

int32 FFightingVRIngameMenu::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}


#undef LOCTEXT_NAMESPACE
