// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRMainMenu.h"
#include "FightingVR.h"
#include "FightingVRStyle.h"
#include "FightingVRMenuSoundsWidgetStyle.h"
#include "FightingVRInstance.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "GenericPlatformChunkInstall.h"
#include "Online/FightingVROnlineGameSettings.h"
#include "OnlineSubsystemSessionSettings.h"
#include "SFightingVRConfirmationDialog.h"
#include "FightingVRMenuItemWidgetStyle.h"
#include "FightingVRUserSettings.h"
#include "FightingVRViewportClient.h"
#include "FightingVRPersistentUser.h"
#include "Player/FightingVRLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

#define MAX_BOT_COUNT 8

static const FString MapNames[] = { TEXT("Sanctuary"), TEXT("Highrise") };
static const FString JoinMapNames[] = { TEXT("Any"), TEXT("Sanctuary"), TEXT("Highrise") };
static const FName PackageNames[] = { TEXT("Sanctuary.umap"), TEXT("Highrise.umap") };
static const int DefaultTDMMap = 1;
static const int DefaultFFAMap = 0; 
static const float QuickmatchUIAnimationTimeDuration = 30.f;

//use an EMap index, get back the ChunkIndex that map should be part of.
//Instead of this mapping we should really use the AssetRegistry to query for chunk mappings, but maps aren't members of the AssetRegistry yet.
static const int ChunkMapping[] = { 1, 2 };

#if PLATFORM_SWITCH
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 1
#else
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 0
#endif

#if PLATFORM_SWITCH
#	define CONSOLE_LAN_SUPPORTED 1
#else
#	define CONSOLE_LAN_SUPPORTED 0
#endif

#if !defined(FightingVR_XBOX_MENU)
	#define FightingVR_XBOX_MENU 0
#endif

FFightingVRMainMenu::~FFightingVRMainMenu()
{
	CleanupOnlinePrivilegeTask();
}

void FFightingVRMainMenu::Construct(TWeakObjectPtr<UFightingVRInstance> _GameInstance, TWeakObjectPtr<ULocalPlayer> _PlayerOwner)
{
	bShowingDownloadPct = false;
	bAnimateQuickmatchSearchingUI = false;
	bUsedInputToCancelQuickmatchSearch = false;
	bQuickmatchSearchRequestCanceled = false;
	bIncQuickMAlpha = false;
	PlayerOwner = _PlayerOwner;
	MatchType = EMatchType::Custom;

	check(_GameInstance.IsValid());

	GameInstance = _GameInstance;
	PlayerOwner = _PlayerOwner;

	OnCancelMatchmakingCompleteDelegate = FOnCancelMatchmakingCompleteDelegate::CreateSP(this, &FFightingVRMainMenu::OnCancelMatchmakingComplete);
	
	// read user settings
#if FIGHTINGVR_CONSOLE_UI
	bIsLanMatch = FParse::Param(FCommandLine::Get(), TEXT("forcelan"));
#else
	UFightingVRUserSettings* const UserSettings = CastChecked<UFightingVRUserSettings>(GEngine->GetGameUserSettings());
	bIsLanMatch = UserSettings->IsLanMatch();
	bIsDedicatedServer = UserSettings->IsDedicatedServer();
#endif

	BotsCountOpt = 1;
	bIsRecordingDemo = false;
	bIsQuitting = false;

	if(GetPersistentUser())
	{
		BotsCountOpt = GetPersistentUser()->GetBotsCount();
		bIsRecordingDemo = GetPersistentUser()->IsRecordingDemos();
	}		

	// number entries 0 up to MAX_BOX_COUNT
	TArray<FText> BotsCountList;
	for (int32 i = 0; i <= MAX_BOT_COUNT; i++)
	{
		BotsCountList.Add(FText::AsNumber(i));
	}
	
	TArray<FText> MapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(MapNames); ++i)
	{
		MapList.Add(FText::FromString(MapNames[i]));		
	}

	TArray<FText> JoinMapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(JoinMapNames); ++i)
	{
		JoinMapList.Add(FText::FromString(JoinMapNames[i]));
	}

	TArray<FText> OnOffList;
	OnOffList.Add( LOCTEXT("Off","OFF") );
	OnOffList.Add( LOCTEXT("On","ON") );

	FightingVROptions = MakeShareable(new FFightingVROptions()); 
	FightingVROptions->Construct(GetPlayerOwner());
	FightingVROptions->TellInputAboutKeybindings();
	FightingVROptions->OnApplyChanges.BindSP(this, &FFightingVRMainMenu::CloseSubMenu);

	//Now that we are here, build our menu 
	MenuWidget.Reset();
	MenuWidgetContainer.Reset();

	TArray<FString> Keys;
	GConfig->GetSingleLineArray(TEXT("/Script/SwitchRuntimeSettings.SwitchRuntimeSettings"), TEXT("LeaderboardMap"), Keys, GEngineIni);

	if (GEngine && GEngine->GameViewport)
	{		
		SAssignNew(MenuWidget, SFightingVRMenuWidget)
			.Cursor(EMouseCursor::Default)
			.PlayerOwner(GetPlayerOwner())
			.IsGameMenu(false);

		SAssignNew(MenuWidgetContainer, SWeakWidget)
			.PossiblyNullContent(MenuWidget);		

		TSharedPtr<FFightingVRMenuItem> RootMenuItem;

				
		SAssignNew(SplitScreenLobbyWidget, SFightingVRSplitScreenLobby)
			.PlayerOwner(GetPlayerOwner())
			.OnCancelClicked(FOnClicked::CreateSP(this, &FFightingVRMainMenu::OnSplitScreenBackedOut)) 
			.OnPlayClicked(FOnClicked::CreateSP(this, &FFightingVRMainMenu::OnSplitScreenPlay));

		FText Msg = LOCTEXT("No matches could be found", "No matches could be found");
		FText OKButtonString = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		QuickMatchFailureWidget = SNew(SFightingVRConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FFightingVRMainMenu::OnQuickMatchFailureUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FFightingVRMainMenu::OnQuickMatchFailureUICancel));

		Msg = LOCTEXT("Searching for Match...", "SEARCHING FOR MATCH...");
		OKButtonString = LOCTEXT("Stop", "STOP");
		QuickMatchSearchingWidget = SNew(SFightingVRConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FFightingVRMainMenu::OnQuickMatchSearchingUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FFightingVRMainMenu::OnQuickMatchSearchingUICancel));

		SAssignNew(SplitScreenLobbyWidgetContainer, SWeakWidget)
			.PossiblyNullContent(SplitScreenLobbyWidget);		

		SAssignNew(QuickMatchFailureWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchFailureWidget);	

		SAssignNew(QuickMatchSearchingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchSearchingWidget);

		FText StoppingOKButtonString = LOCTEXT("Stopping", "STOPPING...");
		QuickMatchStoppingWidget = SNew(SFightingVRConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(StoppingOKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked())
			.OnCancelClicked(FOnClicked());

		SAssignNew(QuickMatchStoppingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchStoppingWidget);

#if FightingVR_XBOX_MENU
		TSharedPtr<FFightingVRMenuItem> MenuItem;

		// HOST ONLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostCustom", "HOST CUSTOM"), this, &FFightingVRMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FFightingVRMainMenu::OnSplitScreenSelected);

			TSharedPtr<FFightingVRMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FFightingVRMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}

		// JOIN menu option
		{
			// JOIN menu option
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("FindCustom", "FIND CUSTOM"), this, &FFightingVRMainMenu::OnJoinServer);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SFightingVRServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}

		// QUICK MATCH menu option
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FFightingVRMainMenu::OnQuickMatchSelected);
		}
#elif FIGHTINGVR_CONSOLE_UI
		TSharedPtr<FFightingVRMenuItem> MenuItem;

#if HOST_ONLINE_GAMEMODE_ENABLED
		// HOST ONLINE menu option
		{
			HostOnlineMenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostOnline", "HOST ONLINE"), this, &FFightingVRMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FFightingVRMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired);
#else
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FFightingVRMainMenu::OnSplitScreenSelectedHostOnline);
#endif

			TSharedPtr<FFightingVRMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FFightingVRMainMenu::BotCountOptionChanged);
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(HostOnlineMenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
#if CONSOLE_LAN_SUPPORTED
			HostLANItem = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FFightingVRMainMenu::LanMatchChanged);
			HostLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}
#endif //HOST_ONLINE_GAMEMODE_ENABLED
		// QUICK MATCH menu option
		{
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FFightingVRMainMenu::OnQuickMatchSelectedLoginRequired);
#else
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FFightingVRMainMenu::OnQuickMatchSelected);
#endif
		}

#if JOIN_ONLINE_GAME_ENABLED
		// JOIN menu option
		{
			// JOIN menu option
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Join", "JOIN"), this, &FFightingVRMainMenu::OnJoinSelected);

			// submenu under "join"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FFightingVRMainMenu::OnJoinServerLoginRequired);
#else
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FFightingVRMainMenu::OnJoinServer);
#endif
			JoinMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), JoinMapList);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SFightingVRServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if CONSOLE_LAN_SUPPORTED
			JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FFightingVRMainMenu::LanMatchChanged);
			JoinLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}
#endif //JOIN_ONLINE_GAME_ENABLED

#else
		TSharedPtr<FFightingVRMenuItem> MenuItem;
		// HOST menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Host", "HOST"));

		// submenu under "host"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("FFALong", "FREE FOR ALL"), this, &FFightingVRMainMenu::OnUIHostFreeForAll);
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FFightingVRMainMenu::OnUIHostTeamDeathMatch);

		TSharedPtr<FFightingVRMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FFightingVRMainMenu::BotCountOptionChanged);
		NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;

		HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);

		HostLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FFightingVRMainMenu::LanMatchChanged);
		HostLANItem->SelectedMultiChoice = bIsLanMatch;

		RecordDemoItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("RecordDemo", "Record Demo"), OnOffList, this, &FFightingVRMainMenu::RecordDemoChanged);
		RecordDemoItem->SelectedMultiChoice = bIsRecordingDemo;

		// JOIN menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Join", "JOIN"));

		// submenu under "join"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FFightingVRMainMenu::OnJoinServer);
		JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FFightingVRMainMenu::LanMatchChanged);
		JoinLANItem->SelectedMultiChoice = bIsLanMatch;

		DedicatedItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("Dedicated", "Dedicated"), OnOffList, this, &FFightingVRMainMenu::DedicatedServerChanged);
		DedicatedItem->SelectedMultiChoice = bIsDedicatedServer;

		// Server list widget that will be called up if appropriate
		MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,SFightingVRServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
#endif

		// Leaderboards
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Leaderboards", "LEADERBOARDS"), this, &FFightingVRMainMenu::OnShowLeaderboard);
		MenuHelper::AddCustomMenuItem(LeaderboardItem,SAssignNew(LeaderboardWidget,SFightingVRLeaderboard).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if ONLINE_STORE_ENABLED
		// Purchases
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Store", "ONLINE STORE"), this, &FFightingVRMainMenu::OnShowOnlineStore);
		MenuHelper::AddCustomMenuItem(OnlineStoreItem, SAssignNew(OnlineStoreWidget, SFightingVROnlineStore).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
#endif //ONLINE_STORE_ENABLED
#if !FIGHTINGVR_CONSOLE_UI

		// Demos
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Demos", "DEMOS"), this, &FFightingVRMainMenu::OnShowDemoBrowser);
			MenuHelper::AddCustomMenuItem(DemoBrowserItem,SAssignNew(DemoListWidget,SFightingVRDemoList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}
#endif

		// Options
		MenuHelper::AddExistingMenuItem(RootMenuItem, FightingVROptions->OptionsItem.ToSharedRef());

		if(FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FFightingVRMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		// QUIT option (for PC)
#if FIGHTINGVR_SHOW_QUIT_MENU_ITEM
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FFightingVRMainMenu::OnUIQuit);
#endif

		MenuWidget->CurrentMenuTitle = LOCTEXT("MainMenu","MAIN MENU");
		MenuWidget->OnGoBack.BindSP(this, &FFightingVRMainMenu::OnMenuGoBack);
		MenuWidget->MainMenu = MenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		MenuWidget->OnMenuHidden.BindSP(this, &FFightingVRMainMenu::OnMenuHidden);

		FightingVROptions->UpdateOptions();
		MenuWidget->BuildAndShowMenu();
	}
}

void FFightingVRMainMenu::AddMenuToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		UGameViewportClient* GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
		GVC->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
	}
}

void FFightingVRMainMenu::RemoveMenuFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
	}
}

void FFightingVRMainMenu::Tick(float DeltaSeconds)
{
	if (bAnimateQuickmatchSearchingUI)
	{
		FLinearColor QuickMColor = QuickMatchSearchingWidget->GetColorAndOpacity();
		if (bIncQuickMAlpha)
		{
			if (QuickMColor.A >= 1.f)
			{
				bIncQuickMAlpha = false;
			}
			else
			{
				QuickMColor.A += DeltaSeconds;
			}
		}
		else
		{
			if (QuickMColor.A <= .1f)
			{
				bIncQuickMAlpha = true;
			}
			else
			{
				QuickMColor.A -= DeltaSeconds;
			}
		}
		QuickMatchSearchingWidget->SetColorAndOpacity(QuickMColor);
		QuickMatchStoppingWidget->SetColorAndOpacity(QuickMColor);
	}

	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// use assetregistry when maps are added to it.
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetPakchunkLocation(MapChunk);

		FText UpdatedText;
		bool bUpdateText = false;
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			float PercentComplete = FMath::Min(ChunkInstaller->GetChunkProgress(MapChunk, EChunkProgressReportingType::PercentageComplete), 100.0f);									
			UpdatedText = FText::FromString(FString::Printf(TEXT("%s %4.0f%%"),*LOCTEXT("SELECTED_LEVEL", "Map").ToString(), PercentComplete));
			bUpdateText = true;
			bShowingDownloadPct = true;
		}
		else if (bShowingDownloadPct)
		{
			UpdatedText = LOCTEXT("SELECTED_LEVEL", "Map");			
			bUpdateText = true;
			bShowingDownloadPct = false;			
		}

		if (bUpdateText)
		{
			if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
			{
				HostOnlineMapOption->SetText(UpdatedText);
			}
			else if (HostOfflineMapOption.IsValid())
			{
				HostOfflineMapOption->SetText(UpdatedText);
			}
		}
	}
}

TStatId FFightingVRMainMenu::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FFightingVRMainMenu, STATGROUP_Tickables);
}

void FFightingVRMainMenu::OnMenuHidden()
{	
#if FIGHTINGVR_CONSOLE_UI
	if (bIsQuitting)
	{
		RemoveMenuFromGameViewport();
	}
	// Menu was hidden from the top-level main menu, on consoles show the welcome screen again.
	else if ( ensure(GameInstance.IsValid()))
	{
		GameInstance->GotoState(FightingVRInstanceState::WelcomeScreen);
	}
#else
	RemoveMenuFromGameViewport();
#endif
}

void FFightingVRMainMenu::OnQuickMatchSelectedLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FFightingVRMainMenu::OnLoginCompleteQuickmatch));
		Identity->Login(ControllerId, FOnlineAccountCredentials());
	}
}

void FFightingVRMainMenu::OnLoginCompleteQuickmatch(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);
	}

	OnQuickMatchSelected();
}

void FFightingVRMainMenu::OnQuickMatchSelected()
{
	bQuickmatchSearchRequestCanceled = false;
#if FIGHTINGVR_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FFightingVRMainMenu::OnUserCanPlayOnlineQuickMatch));
}

void FFightingVRMainMenu::OnUserCanPlayOnlineQuickMatch(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(EOnlineMode::Online);
		}

		MatchType = EMatchType::Quick;

		SplitScreenLobbyWidget->SetIsJoining(false);

		// Skip splitscreen for PS4
#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1
		BeginQuickMatchSearch();
#else
		UGameViewportClient* const GVC = GEngine->GameViewport;

		RemoveMenuFromGameViewport();
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
#endif
	}
	else if (GameInstance.IsValid())
	{

		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

FReply FFightingVRMainMenu::OnConfirmGeneric()
{
	UFightingVRViewportClient* FightingVRViewport = Cast<UFightingVRViewportClient>(GameInstance->GetGameViewportClient());
	if (FightingVRViewport)
	{
		FightingVRViewport->HideDialog();
	}

	return FReply::Handled();
}

void FFightingVRMainMenu::BeginQuickMatchSearch()
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if(!Sessions.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: couldn't find online session interface."));
		return;
	}

	if (GetPlayerOwnerControllerId() == -1)
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: Could not get controller id from player owner"));
		return;
	}

	QuickMatchSearchSettings = MakeShared<FFightingVROnlineSearchSettings>(false, true);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_MATCHMAKING_QUEUE, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_SESSION_TEMPLATE_NAME, FString("MatchSession"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->TimeoutInSeconds = 120.0f;

	FFightingVROnlineSessionSettings SessionSettings(false, true, 8);
	SessionSettings.Set(SETTING_GAMEMODE, FString("TDM"), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
	SessionSettings.Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);

	TSharedRef<FOnlineSessionSearch> QuickMatchSearchSettingsRef = QuickMatchSearchSettings.ToSharedRef();
	
	DisplayQuickmatchSearchingUI();

	// Perform matchmaking with all local players
	TArray<FSessionMatchmakingUser> LocalPlayers;
	for (auto It = GameInstance->GetLocalPlayerIterator(); It; ++It)
	{
		FUniqueNetIdRepl PlayerId = (*It)->GetPreferredUniqueNetId();
		if (PlayerId.IsValid())
		{
			FSessionMatchmakingUser LocalPlayer = {(*PlayerId).AsShared()};
			LocalPlayers.Emplace(LocalPlayer);
		}
	}

	FOnStartMatchmakingComplete CompletionDelegate;
	CompletionDelegate.BindSP(this, &FFightingVRMainMenu::OnMatchmakingComplete);
	if (!Sessions->StartMatchmaking(LocalPlayers, NAME_GameSession, SessionSettings, QuickMatchSearchSettingsRef, CompletionDelegate))
	{
		OnMatchmakingComplete(NAME_GameSession, FOnlineError(false), FSessionMatchmakingResults());
	}
}


void FFightingVRMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnSplitScreenSelected();
		}
		else
		{
			OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FFightingVRMainMenu::OnLoginCompleteHostOnline));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FFightingVRMainMenu::OnSplitScreenSelected()
{
	if (!IsMapReady())
	{
		return;
	}

	RemoveMenuFromGameViewport();

#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1 || !FIGHTINGVR_SUPPORTS_OFFLINE_SPLIT_SCREEEN
	if (!FIGHTINGVR_SUPPORTS_OFFLINE_SPLIT_SCREEEN || (GameInstance.IsValid() && GameInstance->GetOnlineMode() == EOnlineMode::Online))
	{
		OnUIHostTeamDeathMatch();
	}
	else
#endif
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
	}
}

void FFightingVRMainMenu::OnHostOnlineSelected()
{
#if FIGHTINGVR_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MatchType = EMatchType::Custom;

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
	SplitScreenLobbyWidget->SetIsJoining(false);
	MenuWidget->EnterSubMenu();
}

void FFightingVRMainMenu::OnUserCanPlayHostOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)	
	{
		OnSplitScreenSelected();
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FFightingVRMainMenu::OnLoginCompleteHostOnline(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);

	OnSplitScreenSelectedHostOnline();
}

void FFightingVRMainMenu::OnSplitScreenSelectedHostOnline()
{
#if FIGHTINGVR_CONSOLE_UI
	if (!ValidatePlayerForOnlinePlay(GetPlayerOwner()))
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FFightingVRMainMenu::OnUserCanPlayHostOnline));
}
void FFightingVRMainMenu::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	if (GameInstance.IsValid())
	{
		// Lock controls for the duration of the async task
		MenuWidget->LockControls(true);
		FUniqueNetIdRepl UserId;
		if (PlayerOwner.IsValid())
		{
			UserId = PlayerOwner->GetPreferredUniqueNetId();
		}
		GameInstance->StartOnlinePrivilegeTask(Delegate, EUserPrivileges::CanPlayOnline, UserId.GetUniqueNetId());
	}	
}

void FFightingVRMainMenu::CleanupOnlinePrivilegeTask()
{
	if (GameInstance.IsValid())
	{
		GameInstance->CleanupOnlinePrivilegeTask();
	}
}

void FFightingVRMainMenu::OnHostOfflineSelected()
{
	MatchType = EMatchType::Custom;

#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->Logout(GetPlayerOwner()->GetControllerId());
#endif

	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
	SplitScreenLobbyWidget->SetIsJoining( false );

	MenuWidget->EnterSubMenu();
}

FReply FFightingVRMainMenu::OnSplitScreenBackedOut()
{	
	SplitScreenLobbyWidget->Clear();
	SplitScreenBackedOut();
	return FReply::Handled();
}

FReply FFightingVRMainMenu::OnSplitScreenPlay()
{
	switch ( MatchType )
	{
		case EMatchType::Custom:
		{
#if FIGHTINGVR_CONSOLE_UI
			if ( SplitScreenLobbyWidget->GetIsJoining() )
			{
#if 1
				// Until we can make split-screen menu support sub-menus, we need to do it this way
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				AddMenuToGameViewport();

				FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	

				// Grab the map filter if there is one
				FString SelectedMapFilterName = TEXT("ANY");
				if (JoinMapOption.IsValid())
				{
					int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
					if (FilterChoice != INDEX_NONE)
					{
						SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
					}
				}


				MenuWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				MenuWidget->EnterSubMenu();
#else
				SplitScreenLobbyWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				SplitScreenLobbyWidget->EnterSubMenu();
#endif
			}
			else
#endif
			{
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				OnUIHostTeamDeathMatch();
			}
			break;
		}

		case EMatchType::Quick:
		{
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
			}
			BeginQuickMatchSearch();
			break;
		}
	}

	return FReply::Handled();
}

void FFightingVRMainMenu::SplitScreenBackedOut()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());	
	}
	AddMenuToGameViewport();

	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	
}

void FFightingVRMainMenu::HelperQuickMatchSearchingUICancel(bool bShouldRemoveSession)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (bShouldRemoveSession && Sessions.IsValid())
	{
		if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
		{
			UGameViewportClient* const GVC = GEngine->GameViewport;
			GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
			GVC->AddViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
			FSlateApplication::Get().SetKeyboardFocus(QuickMatchStoppingWidgetContainer);
			
			OnCancelMatchmakingCompleteDelegateHandle = Sessions->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegate);
			Sessions->CancelMatchmaking(*PlayerOwner->GetPreferredUniqueNetId(), NAME_GameSession);
		}
	}
	else
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

FReply FFightingVRMainMenu::OnQuickMatchSearchingUICancel()
{
	HelperQuickMatchSearchingUICancel(true);
	bUsedInputToCancelQuickmatchSearch = true;
	bQuickmatchSearchRequestCanceled = true;
	return FReply::Handled();
}

FReply FFightingVRMainMenu::OnQuickMatchFailureUICancel()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	}
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	return FReply::Handled();
}

void FFightingVRMainMenu::DisplayQuickmatchFailureUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchFailureWidget);
}

void FFightingVRMainMenu::DisplayQuickmatchSearchingUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchSearchingWidget);
	bAnimateQuickmatchSearchingUI = true;
}

void FFightingVRMainMenu::OnMatchmakingComplete(FName SessionName, const FOnlineError& ErrorDetails, const FSessionMatchmakingResults& Results)
{
	const bool bWasSuccessful = ErrorDetails.WasSuccessful();
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: Couldn't find session interface."));
		return;
	}

	if (bQuickmatchSearchRequestCanceled && bUsedInputToCancelQuickmatchSearch)
	{
		bQuickmatchSearchRequestCanceled = false;
		// Clean up the session in case we get this event after canceling
		if (bWasSuccessful)
		{
			if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
			{
				SessionInterface->DestroySession(NAME_GameSession);
			}
		}
		return;
	}

	if (bAnimateQuickmatchSearchingUI)
	{
		bAnimateQuickmatchSearchingUI = false;
		HelperQuickMatchSearchingUICancel(false);
		bUsedInputToCancelQuickmatchSearch = false;
	}
	else
	{
		return;
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Matchmaking was unsuccessful."));
		DisplayQuickmatchFailureUI();
		return;
	}

	UE_LOG(LogOnline, Log, TEXT("Matchmaking successful! Session name is %s."), *SessionName.ToString());

	if (GetPlayerOwner() == NULL)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No owner."));
		return;
	}

	FNamedOnlineSession* MatchmadeSession = SessionInterface->GetNamedSession(SessionName);

	if (!MatchmadeSession)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session."));
		return;
	}

	if(!MatchmadeSession->OwningUserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session owner/host."));
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	}
	bAnimateQuickmatchSearchingUI = false;

	UE_LOG(LogOnline, Log, TEXT("OnMatchmakingComplete: Session host is %d."), *MatchmadeSession->OwningUserId->ToString());

	if (ensure(GameInstance.IsValid()))
	{
		MenuWidget->LockControls(true);

		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetTickableGameObjectWorld());
		if (Subsystem != nullptr && Subsystem->IsLocalPlayer(*MatchmadeSession->OwningUserId))
		{
			// This console is the host, start the map.
			GameInstance->BeginHostingQuickMatch();
		}
		else
		{
			// We are the client, join the host.
			GameInstance->TravelToSession(SessionName);
		}
	}
}

FFightingVRMainMenu::EMap FFightingVRMainMenu::GetSelectedMap() const
{
	if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
	{
		return (EMap)HostOnlineMapOption->SelectedMultiChoice;
	}
	else if (HostOfflineMapOption.IsValid())
	{
		return (EMap)HostOfflineMapOption->SelectedMultiChoice;
	}

	return EMap::ESancturary;	// Need to return something (we can hit this path in cooking)
}

void FFightingVRMainMenu::CloseSubMenu()
{
	MenuWidget->MenuGoBack(true);
}

void FFightingVRMainMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (FightingVROptions->OptionsItem->SubMenu == Menu)
	{
		FightingVROptions->RevertChanges();
	}

	// if we've backed all the way out we need to make sure online is false.
	if (MenuWidget->GetMenuLevel() == 1)
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
}

void FFightingVRMainMenu::BotCountOptionChanged(TSharedPtr<FFightingVRMenuItem> MenuItem, int32 MultiOptionIndex)
{
	BotsCountOpt = MultiOptionIndex;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetBotsCount(BotsCountOpt);
	}
}

void FFightingVRMainMenu::LanMatchChanged(TSharedPtr<FFightingVRMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (HostLANItem.IsValid())
	{
		HostLANItem->SelectedMultiChoice = MultiOptionIndex;
	}

	check(JoinLANItem.IsValid());
	JoinLANItem->SelectedMultiChoice = MultiOptionIndex;
	bIsLanMatch = MultiOptionIndex > 0;
	UFightingVRUserSettings* UserSettings = CastChecked<UFightingVRUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetLanMatch(bIsLanMatch);

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
}

void FFightingVRMainMenu::DedicatedServerChanged(TSharedPtr<FFightingVRMenuItem> MenuItem, int32 MultiOptionIndex)
{
	check(DedicatedItem.IsValid());
	DedicatedItem->SelectedMultiChoice = MultiOptionIndex;
	bIsDedicatedServer = MultiOptionIndex > 0;
	UFightingVRUserSettings* UserSettings = CastChecked<UFightingVRUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetDedicatedServer(bIsDedicatedServer);
}

void FFightingVRMainMenu::RecordDemoChanged(TSharedPtr<FFightingVRMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (RecordDemoItem.IsValid())
	{
		RecordDemoItem->SelectedMultiChoice = MultiOptionIndex;
	}

	bIsRecordingDemo = MultiOptionIndex > 0;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetIsRecordingDemos(bIsRecordingDemo);
		GetPersistentUser()->SaveIfDirty();
	}
}

void FFightingVRMainMenu::OnUIHostFreeForAll()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !FIGHTINGVR_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	UWorld* World = GetTickableGameObjectWorld();
	const int32 ControllerId = GetPlayerOwnerControllerId();

	if (World && ControllerId != -1)
	{
		const FFightingVRMenuSoundsStyle& MenuSounds = FFightingVRStyle::Get().GetWidgetStyle<FFightingVRMenuSoundsStyle>("DefaultFightingVRMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.StartGameSound, ControllerId, this, &FFightingVRMainMenu::HostFreeForAll);
	}
}

void FFightingVRMainMenu::OnUIHostTeamDeathMatch()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !FIGHTINGVR_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	if (GetTickableGameObjectWorld() && GetPlayerOwnerControllerId() != -1)
	{
		const FFightingVRMenuSoundsStyle& MenuSounds = FFightingVRStyle::Get().GetWidgetStyle<FFightingVRMenuSoundsStyle>("DefaultFightingVRMenuSoundsStyle");
			MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.StartGameSound, GetPlayerOwnerControllerId(), this, &FFightingVRMainMenu::HostTeamDeathMatch);
	}
}

void FFightingVRMainMenu::HostGame(const FString& GameType)
{	
	if (ensure(GameInstance.IsValid()) && GetPlayerOwner() != NULL)
	{
		FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s%s?%s=%d%s"), *GetMapName(), *GameType, GameInstance->GetOnlineMode() != EOnlineMode::Offline ? TEXT("?listen") : TEXT(""), GameInstance->GetOnlineMode() == EOnlineMode::LAN ? TEXT("?bIsLanMatch") : TEXT(""), *AFightingVRMode::GetBotsCountOptionName(), BotsCountOpt, bIsRecordingDemo ? TEXT("?DemoRec") : TEXT("") );

		// Game instance will handle success, failure and dialogs
		GameInstance->HostGame(GetPlayerOwner(), GameType, StartURL);
	}
}

void FFightingVRMainMenu::HostFreeForAll()
{
	HostGame(TEXT("FFA"));
}

void FFightingVRMainMenu::HostTeamDeathMatch()
{
	HostGame(TEXT("TDM"));
}

FReply FFightingVRMainMenu::OnConfirm()
{
	if (GEngine && GEngine->GameViewport)
	{
		UFightingVRViewportClient * FightingVRViewport = Cast<UFightingVRViewportClient>(GEngine->GameViewport);

		if (FightingVRViewport)
		{
			// Hide the previous dialog
			FightingVRViewport->HideDialog();
		}
	}

	return FReply::Handled();
}

bool FFightingVRMainMenu::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerForOnlinePlay(LocalPlayer);
}

bool FFightingVRMainMenu::ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerIsSignedIn(LocalPlayer);
}

void FFightingVRMainMenu::OnJoinServerLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();
	
		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnUserCanPlayOnlineJoin(*GetPlayerOwner()->GetCachedUniqueNetId(), EUserPrivileges::CanPlayOnline, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
		}
		else
		{
			OnLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FFightingVRMainMenu::OnLoginCompleteJoin));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FFightingVRMainMenu::OnLoginCompleteJoin(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteDelegateHandle);
	}

	OnJoinServer();
}

void FFightingVRMainMenu::OnJoinSelected()
{
#if FIGHTINGVR_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MenuWidget->EnterSubMenu();
}

void FFightingVRMainMenu::OnJoinServer()
{
#if FIGHTINGVR_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FFightingVRMainMenu::OnUserCanPlayOnlineJoin));
}

void FFightingVRMainMenu::OnUserCanPlayOnlineJoin(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{

		//make sure to switch to custom match type so we don't instead use Quick type
		MatchType = EMatchType::Custom;

		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
		}

		MatchType = EMatchType::Custom;
		// Grab the map filter if there is one
		FString SelectedMapFilterName("Any");
		if( JoinMapOption.IsValid())
		{
			int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
			if( FilterChoice != INDEX_NONE )
			{
				SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
			}
		}

#if FIGHTINGVR_CONSOLE_UI
		UGameViewportClient* const GVC = GEngine->GameViewport;
#if PLATFORM_PS4 || MAX_LOCAL_PLAYERS == 1
		// Show server menu (skip splitscreen)
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);

		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#else
		// Show splitscreen menu
		RemoveMenuFromGameViewport();	
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);

		SplitScreenLobbyWidget->SetIsJoining( true );
#endif
#else
		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		//FString SelectedMapFilterName = JoinMapOption->MultiChoice[JoinMapOption->SelectedMultiChoice].ToString();

		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#endif
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FFightingVRMainMenu::OnShowLeaderboard()
{
	MenuWidget->NextMenu = LeaderboardItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	LeaderboardWidget->ReadStatsLoginRequired();
#else
	LeaderboardWidget->ReadStats();
#endif
	MenuWidget->EnterSubMenu();
}

void FFightingVRMainMenu::OnShowOnlineStore()
{
	MenuWidget->NextMenu = OnlineStoreItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	UE_LOG(LogOnline, Warning, TEXT("You need to be logged in before using the store"));
#endif
	OnlineStoreWidget->BeginGettingOffers();
	MenuWidget->EnterSubMenu();
}

void FFightingVRMainMenu::OnShowDemoBrowser()
{
	MenuWidget->NextMenu = DemoBrowserItem->SubMenu;
	DemoListWidget->BuildDemoList();
	MenuWidget->EnterSubMenu();
}

void FFightingVRMainMenu::OnUIQuit()
{
	bIsQuitting = true;
	LockAndHideMenu();

	const FFightingVRMenuSoundsStyle& MenuSounds = FFightingVRStyle::Get().GetWidgetStyle<FFightingVRMenuSoundsStyle>("DefaultFightingVRMenuSoundsStyle");

	if (GetTickableGameObjectWorld() != NULL && GetPlayerOwnerControllerId() != -1)
	{
		FSlateApplication::Get().PlaySound(MenuSounds.ExitGameSound, GetPlayerOwnerControllerId());
		MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.ExitGameSound, GetPlayerOwnerControllerId(), this, &FFightingVRMainMenu::Quit);
	}
}

void FFightingVRMainMenu::Quit()
{
	if (ensure(GameInstance.IsValid()))
	{
		UGameViewportClient* const Viewport = GameInstance->GetGameViewportClient();
		if (ensure(Viewport)) 
		{
			Viewport->ConsoleCommand("quit");
		}
	}
}

void FFightingVRMainMenu::LockAndHideMenu()
{
	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();
}

void FFightingVRMainMenu::DisplayLoadingScreen()
{
	
}

bool FFightingVRMainMenu::IsMapReady() const
{
	bool bReady = true;
	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// should use the AssetRegistry as soon as maps are added to the AssetRegistry
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetPakchunkLocation(MapChunk);
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			bReady = false;
		}
	}
	return bReady;
}

UFightingVRPersistentUser* FFightingVRMainMenu::GetPersistentUser() const
{
	UFightingVRLocalPlayer* const FightingVRLocalPlayer = Cast<UFightingVRLocalPlayer>(GetPlayerOwner());
	return FightingVRLocalPlayer ? FightingVRLocalPlayer->GetPersistentUser() : nullptr;
}

UWorld* FFightingVRMainMenu::GetTickableGameObjectWorld() const
{
	ULocalPlayer* LocalPlayerOwner = GetPlayerOwner();
	return (LocalPlayerOwner ? LocalPlayerOwner->GetWorld() : nullptr);
}

ULocalPlayer* FFightingVRMainMenu::GetPlayerOwner() const
{
	return PlayerOwner.Get();
}

int32 FFightingVRMainMenu::GetPlayerOwnerControllerId() const
{
	return ( PlayerOwner.IsValid() ) ? PlayerOwner->GetControllerId() : -1;
}

FString FFightingVRMainMenu::GetMapName() const
{
	return MapNames[(int)GetSelectedMap()];
}

void FFightingVRMainMenu::OnCancelMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);
	}

	bAnimateQuickmatchSearchingUI = false;
	UGameViewportClient* const GVC = GEngine->GameViewport;
	GVC->RemoveViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
}

#undef LOCTEXT_NAMESPACE
