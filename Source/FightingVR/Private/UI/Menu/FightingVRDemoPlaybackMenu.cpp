// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRDemoPlaybackMenu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "FightingVRMenuSoundsWidgetStyle.h"
#include "FightingVRInstance.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

void FFightingVRDemoPlaybackMenu::Construct( ULocalPlayer* _PlayerOwner )
{
	PlayerOwner = _PlayerOwner;
	bIsAddedToViewport = false;

	if ( !GEngine || !GEngine->GameViewport )
	{
		return;
	}
	
	if ( !GameMenuWidget.IsValid() )
	{
		SAssignNew( GameMenuWidget, SFightingVRMenuWidget )
			.PlayerOwner( MakeWeakObjectPtr( PlayerOwner ) )
			.Cursor( EMouseCursor::Default )
			.IsGameMenu( true );			

		TSharedPtr<FFightingVRMenuItem> MainMenuRoot = FFightingVRMenuItem::CreateRoot();
		MainMenuItem = MenuHelper::AddMenuItem(MainMenuRoot,LOCTEXT( "Main Menu", "MAIN MENU" ) );

		MenuHelper::AddMenuItemSP( MainMenuItem, LOCTEXT( "No", "NO" ), this, &FFightingVRDemoPlaybackMenu::OnCancelExitToMain );
		MenuHelper::AddMenuItemSP( MainMenuItem, LOCTEXT( "Yes", "YES" ), this, &FFightingVRDemoPlaybackMenu::OnConfirmExitToMain );

		MenuHelper::AddExistingMenuItem( RootMenuItem, MainMenuItem.ToSharedRef() );
				
#if !FIGHTINGVR_CONSOLE_UI
		MenuHelper::AddMenuItemSP( RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FFightingVRDemoPlaybackMenu::OnUIQuit );
#endif

		GameMenuWidget->MainMenu = GameMenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		GameMenuWidget->OnMenuHidden.BindSP( this, &FFightingVRDemoPlaybackMenu::DetachGameMenu );
		GameMenuWidget->OnToggleMenu.BindSP( this, &FFightingVRDemoPlaybackMenu::ToggleGameMenu );
		GameMenuWidget->OnGoBack.BindSP( this, &FFightingVRDemoPlaybackMenu::OnMenuGoBack );
	}
}

void FFightingVRDemoPlaybackMenu::CloseSubMenu()
{
	GameMenuWidget->MenuGoBack();
}

void FFightingVRDemoPlaybackMenu::OnMenuGoBack(MenuPtr Menu)
{
}

void FFightingVRDemoPlaybackMenu::DetachGameMenu()
{
	if ( GEngine && GEngine->GameViewport )
	{
		GEngine->GameViewport->RemoveViewportWidgetContent( GameMenuContainer.ToSharedRef() );
	}

	bIsAddedToViewport = false;
}

void FFightingVRDemoPlaybackMenu::ToggleGameMenu()
{
	if ( !GameMenuWidget.IsValid( ))
	{
		return;
	}

	if ( bIsAddedToViewport && GameMenuWidget->CurrentMenu != RootMenuItem->SubMenu )
	{
		GameMenuWidget->MenuGoBack();
		return;
	}
	
	if ( !bIsAddedToViewport )
	{
		GEngine->GameViewport->AddViewportWidgetContent( SAssignNew( GameMenuContainer, SWeakWidget ).PossiblyNullContent( GameMenuWidget.ToSharedRef() ) );

		GameMenuWidget->BuildAndShowMenu();

		bIsAddedToViewport = true;
	} 
	else
	{
		// Start hiding animation
		GameMenuWidget->HideMenu();

		AFightingVRPlayerController* const PCOwner = PlayerOwner ? Cast<AFightingVRPlayerController>(PlayerOwner->PlayerController) : nullptr;

		if ( PCOwner )
		{
			// Make sure viewport has focus
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

void FFightingVRDemoPlaybackMenu::OnCancelExitToMain()
{
	CloseSubMenu();
}

void FFightingVRDemoPlaybackMenu::OnConfirmExitToMain()
{
	UFightingVRInstance* const GameInstance = Cast<UFightingVRInstance>( PlayerOwner->GetGameInstance() );

	if ( GameInstance )
	{
		// tell game instance to go back to main menu state
		GameInstance->GotoState( FightingVRInstanceState::MainMenu );
	}
}

void FFightingVRDemoPlaybackMenu::OnUIQuit()
{
	UFightingVRInstance* const GameInstance = Cast<UFightingVRInstance>( PlayerOwner->GetGameInstance() );

	GameMenuWidget->LockControls( true );
	GameMenuWidget->HideMenu();

	UWorld* const World = PlayerOwner ? PlayerOwner->GetWorld() : nullptr;
	if ( World )
	{
		const FFightingVRMenuSoundsStyle& MenuSounds = FFightingVRStyle::Get().GetWidgetStyle< FFightingVRMenuSoundsStyle >( "DefaultFightingVRMenuSoundsStyle" );
		MenuHelper::PlaySoundAndCall( World, MenuSounds.ExitGameSound, GetOwnerUserIndex(), this, &FFightingVRDemoPlaybackMenu::Quit );
	}
}

void FFightingVRDemoPlaybackMenu::Quit()
{
	APlayerController* const PCOwner = PlayerOwner ? PlayerOwner->PlayerController : nullptr;

	if ( PCOwner )
	{
		PCOwner->ConsoleCommand( "quit" );
	}
}

int32 FFightingVRDemoPlaybackMenu::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}
#undef LOCTEXT_NAMESPACE
