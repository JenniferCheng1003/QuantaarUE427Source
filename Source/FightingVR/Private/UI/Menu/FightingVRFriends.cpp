// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRFriends.h"
#include "FightingVR.h"
#include "FightingVRTypes.h"
#include "FightingVRStyle.h"
#include "FightingVROptionsWidgetStyle.h"
#include "Player/FightingVRPersistentUser.h"
#include "FightingVRUserSettings.h"
#include "FightingVRLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

void FFightingVRFriends::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	FriendsStyle = &FFightingVRStyle::Get().GetWidgetStyle<FFightingVROptionsStyle>("DefaultFightingVROptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrFriendIndex = 0;
	MinFriendIndex = 0;
	MaxFriendIndex = 0; //initialized after the friends list is read in

	/** Friends menu root item */
	TSharedPtr<FFightingVRMenuItem> FriendsRoot = FFightingVRMenuItem::CreateRoot();

	//Populate the friends list
	FriendsItem = MenuHelper::AddMenuItem(FriendsRoot, LOCTEXT("Friends", "FRIENDS"));

	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
	}

	UpdateFriends(LocalUserNum);

	UserSettings = CastChecked<UFightingVRUserSettings>(GEngine->GetGameUserSettings());
}

void FFightingVRFriends::OnApplySettings()
{
	ApplySettings();
}

void FFightingVRFriends::ApplySettings()
{
	UFightingVRPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();

		PersistentUser->SaveIfDirty();
	}

	UserSettings->ApplySettings(false);

	OnApplyChanges.ExecuteIfBound();
}

void FFightingVRFriends::TellInputAboutKeybindings()
{
	UFightingVRPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

UFightingVRPersistentUser* FFightingVRFriends::GetPersistentUser() const
{
	UFightingVRLocalPlayer* const FightingVRLocalPlayer = Cast<UFightingVRLocalPlayer>(PlayerOwner);
	return FightingVRLocalPlayer ? FightingVRLocalPlayer->GetPersistentUser() : nullptr;
	//// Main Menu
	//AFightingVRPlayerController_Menu* FightingVRPCM = Cast<AFightingVRPlayerController_Menu>(PCOwner);
	//if(FightingVRPCM)
	//{
	//	return FightingVRPCM->GetPersistentUser();
	//}

	//// In-game Menu
	//AFightingVRPlayerController* FightingVRPC = Cast<AFightingVRPlayerController>(PCOwner);
	//if(FightingVRPC)
	//{
	//	return FightingVRPC->GetPersistentUser();
	//}

	//return nullptr;
}

void FFightingVRFriends::UpdateFriends(int32 NewOwnerIndex)
{
	if (!OnlineFriendsPtr.IsValid())
	{
		return;
	}

	LocalUserNum = NewOwnerIndex;
	OnlineFriendsPtr->ReadFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), FOnReadFriendsListComplete::CreateSP(this, &FFightingVRFriends::OnFriendsUpdated));
}

void FFightingVRFriends::OnFriendsUpdated(int32 /*unused*/, bool bWasSuccessful, const FString& FriendListName, const FString& ErrorString)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Unable to update friendslist %s due to error=[%s]"), *FriendListName, *ErrorString);
		return;
	}

	MenuHelper::ClearSubMenu(FriendsItem);

	Friends.Reset();
	if (OnlineFriendsPtr->GetFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), Friends))
	{
		for (const TSharedRef<FOnlineFriend>& Friend : Friends)
		{
			TSharedRef<FFightingVRMenuItem> FriendItem = MenuHelper::AddMenuItem(FriendsItem, FText::FromString(Friend->GetDisplayName()));
			FriendItem->OnControllerFacebuttonDownPressed.BindSP(this, &FFightingVRFriends::ViewSelectedFriendProfile);
			FriendItem->OnControllerDownInputPressed.BindSP(this, &FFightingVRFriends::IncrementFriendsCounter);
			FriendItem->OnControllerUpInputPressed.BindSP(this, &FFightingVRFriends::DecrementFriendsCounter);
		}

		MaxFriendIndex = Friends.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(FriendsItem, LOCTEXT("Close", "CLOSE"), this, &FFightingVRFriends::OnApplySettings);
}

void FFightingVRFriends::IncrementFriendsCounter()
{
	if (CurrFriendIndex + 1 <= MaxFriendIndex)
	{
		++CurrFriendIndex;
	}
}
void FFightingVRFriends::DecrementFriendsCounter()
{
	if (CurrFriendIndex - 1 >= MinFriendIndex)
	{
		--CurrFriendIndex;
	}
}
void FFightingVRFriends::ViewSelectedFriendProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && Friends.IsValidIndex(CurrFriendIndex))
		{
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = Friends[CurrFriendIndex]->GetUserId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
}
}
void FFightingVRFriends::InviteSelectedFriendToGame()
{
	// invite the user to the current gamesession
	if (OnlineSub)
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSub->GetSessionInterface();
		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->SendSessionInviteToFriend(LocalUserNum, NAME_GameSession, *Friends[CurrFriendIndex]->GetUserId());
		}
	}
}


#undef LOCTEXT_NAMESPACE

