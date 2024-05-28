// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRLocalPlayer.h"
#include "FightingVR.h"
#include "OnlineSubsystemUtilsClasses.h"
#include "FightingVRInstance.h"
#include "OnlineSubsystemUtils.h"

UFightingVRLocalPlayer::UFightingVRLocalPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UFightingVRPersistentUser* UFightingVRLocalPlayer::GetPersistentUser() const
{
	// if persistent data isn't loaded yet, load it
	if (PersistentUser == nullptr)
	{
		UFightingVRLocalPlayer* const MutableThis = const_cast<UFightingVRLocalPlayer*>(this);
		// casting away constness to enable caching implementation behavior
		MutableThis->LoadPersistentUser();
	}
	return PersistentUser;
}

void UFightingVRLocalPlayer::LoadPersistentUser()
{
	FString SaveGameName = GetNickname();

#if PLATFORM_SWITCH
	// on Switch, the displayable nickname can change, so we can't use it as a save ID (explicitly stated in docs, so changing for pre-cert)
	FPlatformMisc::GetUniqueStringNameForControllerId(GetControllerId(), SaveGameName);
#endif

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || SaveGameName != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (PersistentUser == NULL)
	{
		// Use the platform id here to be resilient in the face of controller swapping and similar situations.
		FPlatformUserId PlatformId = GetControllerId();

		IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetWorld());
		if (Identity.IsValid() && GetPreferredUniqueNetId().IsValid())
		{
			PlatformId = Identity->GetPlatformUserIdFromUniqueNetId(*GetPreferredUniqueNetId());
		}

		PersistentUser = UFightingVRPersistentUser::LoadPersistentUser(SaveGameName, PlatformId );
	}
}

void UFightingVRLocalPlayer::SetControllerId(int32 NewControllerId)
{
	ULocalPlayer::SetControllerId(NewControllerId);

	FString SaveGameName = GetNickname();

#if PLATFORM_SWITCH
	// on Switch, the displayable nickname can change, so we can't use it as a save ID (explicitly stated in docs, so changing for pre-cert)
	FPlatformMisc::GetUniqueStringNameForControllerId(GetControllerId(), SaveGameName);
#endif

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || SaveGameName != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (!PersistentUser)
	{
		LoadPersistentUser();
	}
}

FString UFightingVRLocalPlayer::GetNickname() const
{
	FString UserNickName = Super::GetNickname();

	if ( UserNickName.Len() > MAX_PLAYER_NAME_LENGTH )
	{
		UserNickName = UserNickName.Left( MAX_PLAYER_NAME_LENGTH ) + "...";
	}

	bool bReplace = (UserNickName.Len() == 0);

	// Check for duplicate nicknames...and prevent reentry
	static bool bReentry = false;
	if(!bReentry)
	{
		bReentry = true;
		UFightingVRInstance* GameInstance = GetWorld() != NULL ? Cast<UFightingVRInstance>(GetWorld()->GetGameInstance()) : NULL;
		if(GameInstance)
		{
			// Check all the names that occur before ours that are the same
			const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
			for (int i = 0; i < LocalPlayers.Num(); ++i)
			{
				const ULocalPlayer* LocalPlayer = LocalPlayers[i];
				if( this == LocalPlayer)
				{
					break;
				}

				if( UserNickName == LocalPlayer->GetNickname())
				{
					bReplace = true;
					break;
				}
			}
		}
		bReentry = false;
	}

	if ( bReplace )
	{
		UserNickName = FString::Printf( TEXT( "Player%i" ), GetControllerId() + 1 );
	}	

	return UserNickName;
}
