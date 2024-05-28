// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRRecentlyMet.h"
#include "FightingVR.h"
#include "FightingVRTypes.h"
#include "FightingVRStyle.h"
#include "FightingVROptionsWidgetStyle.h"
#include "FightingVRUserSettings.h"
#include "FightingVRPersistentUser.h"
#include "Player/FightingVRLocalPlayer.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "FightingVR.HUD.Menu"

void FFightingVRRecentlyMet::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	RecentlyMetStyle = &FFightingVRStyle::Get().GetWidgetStyle<FFightingVROptionsStyle>("DefaultFightingVROptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrRecentlyMetIndex = 0;
	MinRecentlyMetIndex = 0;
	MaxRecentlyMetIndex = 0; //initialized after the recently met list is (read in/set)

	/** Recently Met menu items */
	RecentlyMetRoot = FFightingVRMenuItem::CreateRoot();
	RecentlyMetItem = MenuHelper::AddMenuItem(RecentlyMetRoot, LOCTEXT("Recently Met", "RECENTLY MET"));

	/** Init online items */
	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
	}

	UserSettings = CastChecked<UFightingVRUserSettings>(GEngine->GetGameUserSettings());	
}

void FFightingVRRecentlyMet::OnApplySettings()
{
	ApplySettings();
}

void FFightingVRRecentlyMet::ApplySettings()
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

void FFightingVRRecentlyMet::TellInputAboutKeybindings()
{
	UFightingVRPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

UFightingVRPersistentUser* FFightingVRRecentlyMet::GetPersistentUser() const
{
	UFightingVRLocalPlayer* const SLP = Cast<UFightingVRLocalPlayer>(PlayerOwner);
	return SLP ? SLP->GetPersistentUser() : nullptr;
}

void FFightingVRRecentlyMet::UpdateRecentlyMet(int32 NewOwnerIndex)
{
	LocalUserNum = NewOwnerIndex;
	
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid())
		{
			LocalUsername = Identity->GetPlayerNickname(LocalUserNum);
		}
	}
	
	MenuHelper::ClearSubMenu(RecentlyMetItem);
	MaxRecentlyMetIndex = 0;

	AFightingVRState* const MyGameState = PlayerOwner->GetWorld()->GetGameState<AFightingVRState>();
	if (MyGameState != nullptr)
	{
		MetPlayerArray = MyGameState->PlayerArray;

		for (int32 i = 0; i < MetPlayerArray.Num(); ++i)
		{
			const APlayerState* PlayerState = MetPlayerArray[i];
			FString Username = PlayerState->GetHumanReadableName();
			if (Username != LocalUsername && PlayerState->IsABot() == false)
			{
				TSharedPtr<FFightingVRMenuItem> UserItem = MenuHelper::AddMenuItem(RecentlyMetItem, FText::FromString(Username));
				UserItem->OnControllerDownInputPressed.BindRaw(this, &FFightingVRRecentlyMet::IncrementRecentlyMetCounter);
				UserItem->OnControllerUpInputPressed.BindRaw(this, &FFightingVRRecentlyMet::DecrementRecentlyMetCounter);
				UserItem->OnControllerFacebuttonDownPressed.BindRaw(this, &FFightingVRRecentlyMet::ViewSelectedUsersProfile);
			}
			else
			{
				MetPlayerArray.RemoveAt(i);
				--i; //we just deleted an item, so we need to go make sure i doesn't increment again, otherwise it would skip the player that was supposed to be looked at next
			}
		}

		MaxRecentlyMetIndex = MetPlayerArray.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(RecentlyMetItem, LOCTEXT("Close", "CLOSE"), this, &FFightingVRRecentlyMet::OnApplySettings);
}

void FFightingVRRecentlyMet::IncrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex + 1 <= MaxRecentlyMetIndex)
	{
		++CurrRecentlyMetIndex;
	}
}
void FFightingVRRecentlyMet::DecrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex - 1 >= MinRecentlyMetIndex)
	{
		--CurrRecentlyMetIndex;
	}
}
void FFightingVRRecentlyMet::ViewSelectedUsersProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && MetPlayerArray.IsValidIndex(CurrRecentlyMetIndex))
		{
			const APlayerState* PlayerState = MetPlayerArray[CurrRecentlyMetIndex];
		
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = PlayerState->GetUniqueId().GetUniqueNetId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
