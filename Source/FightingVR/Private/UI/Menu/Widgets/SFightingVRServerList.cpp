// Copyright Epic Games, Inc. All Rights Reserved.

#include "SFightingVRServerList.h"
#include "FightingVR.h"
#include "SHeaderRow.h"
#include "FightingVRStyle.h"
#include "FightingVRInstance.h"
#include "Online/FightingVRSession.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

void SFightingVRServerList::Construct(const FArguments& InArgs)
{
	PlayerOwner = InArgs._PlayerOwner;
	OwnerWidget = InArgs._OwnerWidget;
	MapFilterName = "Any";
	bSearchingForServers = false;
	bLANMatchSearch = false;
	StatusText = FText::GetEmpty();
	BoxWidth = 125;
	LastSearchTime = 0.0f;
	
#if PLATFORM_SWITCH
	MinTimeBetweenSearches = 6.0;
#else
	MinTimeBetweenSearches = 0.0;
#endif

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)  
			.WidthOverride(600)
			.HeightOverride(300)
			[
				SAssignNew(ServerListWidget, SListView<TSharedPtr<FServerEntry>>)
				.ItemHeight(20)
				.ListItemsSource(&ServerList)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SFightingVRServerList::MakeListViewWidget)
				.OnSelectionChanged(this, &SFightingVRServerList::EntrySelectionChanged)
				.OnMouseButtonDoubleClick(this,&SFightingVRServerList::OnListItemDoubleClicked)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("ServerName").FixedWidth(BoxWidth*2) .DefaultLabel(NSLOCTEXT("ServerList", "ServerNameColumn", "Server Name"))
					+ SHeaderRow::Column("GameType") .DefaultLabel(NSLOCTEXT("ServerList", "GameTypeColumn", "Game Type"))
					+ SHeaderRow::Column("Map").DefaultLabel(NSLOCTEXT("ServerList", "MapNameColumn", "Map"))
					+ SHeaderRow::Column("Players") .DefaultLabel(NSLOCTEXT("ServerList", "PlayersColumn", "Players"))
					+ SHeaderRow::Column("Ping") .DefaultLabel(NSLOCTEXT("ServerList", "NetworkPingColumn", "Ping")))
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SRichTextBlock)
				.Text(this, &SFightingVRServerList::GetBottomText)
				.TextStyle(FFightingVRStyle::Get(), "FightingVR.MenuServerListTextStyle")
				.DecoratorStyleSet(&FFightingVRStyle::Get())
				+ SRichTextBlock::ImageDecorator()
			]
		]
		
	];
}

/** 
 * Get the current game session
 */
AFightingVRSession* SFightingVRServerList::GetGameSession() const
{
	UFightingVRInstance* const GI = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
	return GI ? GI->GetGameSession() : nullptr;
}

/** Updates current search status */
void SFightingVRServerList::UpdateSearchStatus()
{
	check(bSearchingForServers); // should not be called otherwise

	bool bFinishSearch = true;
	AFightingVRSession* FightingVRSession = GetGameSession();
	if (FightingVRSession)
	{
		int32 CurrentSearchIdx, NumSearchResults;
		EOnlineAsyncTaskState::Type SearchState = FightingVRSession->GetSearchResultStatus(CurrentSearchIdx, NumSearchResults);

		UE_LOG(LogOnlineGame, Log, TEXT("FightingVRSession->GetSearchResultStatus: %s"), EOnlineAsyncTaskState::ToString(SearchState) );

		switch(SearchState)
		{
			case EOnlineAsyncTaskState::InProgress:
				StatusText = LOCTEXT("Searching","SEARCHING...");
				bFinishSearch = false;
				break;

			case EOnlineAsyncTaskState::Done:
				// copy the results
				{
					ServerList.Empty();
					const TArray<FOnlineSessionSearchResult> & SearchResults = FightingVRSession->GetSearchResults();
					check(SearchResults.Num() == NumSearchResults);
					if (NumSearchResults == 0)
					{
#if PLATFORM_PS4
						StatusText = LOCTEXT("NoServersFound","NO SERVERS FOUND, PRESS SQUARE TO TRY AGAIN");
#elif FIGHTINGVR_XBOX_STRINGS
						StatusText = LOCTEXT("NoServersFound","NO SERVERS FOUND, PRESS X TO TRY AGAIN");
#elif PLATFORM_SWITCH
						StatusText = LOCTEXT("NoServersFound", "NO SERVERS FOUND, PRESS <img src=\"FightingVR.Switch.Left\"/> TO TRY AGAIN");
#else
						StatusText = LOCTEXT("NoServersFound","NO SERVERS FOUND, PRESS SPACE TO TRY AGAIN");
#endif
					}
					else
					{
#if PLATFORM_PS4
						StatusText = LOCTEXT("ServersRefresh","PRESS SQUARE TO REFRESH SERVER LIST");
#elif FIGHTINGVR_XBOX_STRINGS
						StatusText = LOCTEXT("ServersRefresh","PRESS X TO REFRESH SERVER LIST");
#elif PLATFORM_SWITCH
						StatusText = LOCTEXT("ServersRefresh", "PRESS <img src=\"FightingVR.Switch.Left\"/> TO REFRESH SERVER LIST");
#else
						StatusText = LOCTEXT("ServersRefresh","PRESS SPACE TO REFRESH SERVER LIST");
#endif
					}

					for (int32 IdxResult = 0; IdxResult < NumSearchResults; ++IdxResult)
					{
						TSharedPtr<FServerEntry> NewServerEntry = MakeShareable(new FServerEntry());

						const FOnlineSessionSearchResult& Result = SearchResults[IdxResult];

						NewServerEntry->ServerName = Result.Session.OwningUserName;
						NewServerEntry->Ping = FString::FromInt(Result.PingInMs);
						NewServerEntry->CurrentPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections 
							+ Result.Session.SessionSettings.NumPrivateConnections 
							- Result.Session.NumOpenPublicConnections 
							- Result.Session.NumOpenPrivateConnections);
						NewServerEntry->MaxPlayers = FString::FromInt(Result.Session.SessionSettings.NumPublicConnections
							+ Result.Session.SessionSettings.NumPrivateConnections);
						NewServerEntry->SearchResultsIndex = IdxResult;
					
						Result.Session.SessionSettings.Get(SETTING_GAMEMODE, NewServerEntry->GameType);
						Result.Session.SessionSettings.Get(SETTING_MAPNAME, NewServerEntry->MapName);

						ServerList.Add(NewServerEntry);
					}
				}
				break;

			case EOnlineAsyncTaskState::Failed:
				// intended fall-through
			case EOnlineAsyncTaskState::NotStarted:
				StatusText = FText::GetEmpty();
				// intended fall-through
			default:
				break;
		}
	}

	if (bFinishSearch)
	{		
		OnServerSearchFinished();
	}
}


FText SFightingVRServerList::GetBottomText() const
{
	 return StatusText;
}

/**
 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
 *
 * @param  InCurrentTime  Current absolute real time
 * @param  InDeltaTime  Real time passed since last tick
 */
void SFightingVRServerList::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

	if ( bSearchingForServers )
	{
		UpdateSearchStatus();
	}
}

/** Starts searching for servers */
void SFightingVRServerList::BeginServerSearch(bool bLANMatch, bool bIsDedicatedServer, const FString& InMapFilterName)
{
	double CurrentTime = FApp::GetCurrentTime();
	if (!bLANMatch && CurrentTime - LastSearchTime < MinTimeBetweenSearches)
	{
		OnServerSearchFinished();
	}
	else
	{
		bLANMatchSearch = bLANMatch;
		bDedicatedServer = bIsDedicatedServer;
		MapFilterName = InMapFilterName;
		bSearchingForServers = true;
		ServerList.Empty();
		LastSearchTime = CurrentTime;

		UFightingVRInstance* const GI = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
		if (GI)
		{
			GI->FindSessions(PlayerOwner.Get(), bIsDedicatedServer, bLANMatchSearch);
		}
	}
}

/** Called when server search is finished */
void SFightingVRServerList::OnServerSearchFinished()
{
	bSearchingForServers = false;

	UpdateServerList();
}

void SFightingVRServerList::UpdateServerList()
{
	/** Only filter maps if a specific map is specified */
	if (MapFilterName != "Any")
	{
		for (int32 i = 0; i < ServerList.Num(); ++i)
		{
			/** Only filter maps if a specific map is specified */
			if (ServerList[i]->MapName != MapFilterName)
			{
				ServerList.RemoveAt(i);
				i--;
			}
		}
	}

	int32 SelectedItemIndex = ServerList.IndexOfByKey(SelectedItem);

	ServerListWidget->RequestListRefresh();
	if (ServerList.Num() > 0)
	{
		ServerListWidget->UpdateSelectionSet();
		ServerListWidget->SetSelection(ServerList[SelectedItemIndex > -1 ? SelectedItemIndex : 0],ESelectInfo::OnNavigation);
	}

}

void SFightingVRServerList::ConnectToServer()
{
	if (bSearchingForServers)
	{
		// unsafe
		return;
	}
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (SelectedItem.IsValid())
	{
		int ServerToJoin = SelectedItem->SearchResultsIndex;

		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveAllViewportWidgets();
		}
		
		UFightingVRInstance* const GI = Cast<UFightingVRInstance>(PlayerOwner->GetGameInstance());
		if (GI)
		{
			GI->JoinSession(PlayerOwner.Get(), ServerToJoin);
		}
	}
}

void SFightingVRServerList::OnFocusLost( const FFocusEvent& InFocusEvent )
{
	if (InFocusEvent.GetCause() != EFocusCause::SetDirectly)
	{
		FSlateApplication::Get().SetKeyboardFocus(SharedThis( this ));
	}
}

FReply SFightingVRServerList::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	return FReply::Handled().SetUserFocus(ServerListWidget.ToSharedRef(), EFocusCause::SetDirectly).SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

void SFightingVRServerList::EntrySelectionChanged(TSharedPtr<FServerEntry> InItem, ESelectInfo::Type SelectInfo)
{
	SelectedItem = InItem;
}

void SFightingVRServerList::OnListItemDoubleClicked(TSharedPtr<FServerEntry> InItem)
{
	SelectedItem = InItem;
	ConnectToServer();
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

void SFightingVRServerList::MoveSelection(int32 MoveBy)
{
	int32 SelectedItemIndex = ServerList.IndexOfByKey(SelectedItem);

	if (SelectedItemIndex+MoveBy > -1 && SelectedItemIndex+MoveBy < ServerList.Num())
	{
		ServerListWidget->SetSelection(ServerList[SelectedItemIndex+MoveBy]);
	}
}

FReply SFightingVRServerList::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) 
{
	if (bSearchingForServers) // lock input
	{
		return FReply::Handled();
	}

	FReply Result = FReply::Unhandled();
	const FKey Key = InKeyEvent.GetKey();
	
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		MoveSelection(-1);
		Result = FReply::Handled();
	}
	else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		MoveSelection(1);
		Result = FReply::Handled();
	}
	else if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
	{
		ConnectToServer();
		Result = FReply::Handled();
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
	}
	//hit space bar to search for servers again / refresh the list, only when not searching already
	else if (Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Left)
	{
		BeginServerSearch(bLANMatchSearch, bDedicatedServer, MapFilterName);
	}
	return Result;
}

TSharedRef<ITableRow> SFightingVRServerList::MakeListViewWidget(TSharedPtr<FServerEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	class SServerEntryWidget : public SMultiColumnTableRow< TSharedPtr<FServerEntry> >
	{
	public:
		SLATE_BEGIN_ARGS(SServerEntryWidget){}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FServerEntry> InItem)
		{
			Item = InItem;
			SMultiColumnTableRow< TSharedPtr<FServerEntry> >::Construct(FSuperRowType::FArguments(), InOwnerTable);
		}

		TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName)
		{
			FText ItemText = FText::GetEmpty();
			if (ColumnName == "ServerName")
			{
				ItemText = FText::FromString(Item->ServerName + "'s Server");
			}
			else if (ColumnName == "GameType")
			{
				ItemText = FText::FromString(Item->GameType);
			}
			else if (ColumnName == "Map")
			{
				ItemText = FText::FromString(Item->MapName);
			}
			else if (ColumnName == "Players")
			{
				ItemText = FText::Format( FText::FromString("{0}/{1}"), FText::FromString(Item->CurrentPlayers), FText::FromString(Item->MaxPlayers) );
			}
			else if (ColumnName == "Ping")
			{
				ItemText = FText::FromString(Item->Ping);
			} 
			return SNew(STextBlock)
				.Text(ItemText)
				.TextStyle(FFightingVRStyle::Get(), "FightingVR.MenuServerListTextStyle");
		}
		TSharedPtr<FServerEntry> Item;
	};
	return SNew(SServerEntryWidget, OwnerTable, Item);
}

#undef LOCTEXT_NAMESPACE