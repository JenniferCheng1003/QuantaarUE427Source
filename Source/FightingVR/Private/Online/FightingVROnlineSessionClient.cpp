// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVROnlineSessionClient.h"
#include "FightingVR.h"
#include "FightingVRInstance.h"

UFightingVROnlineSessionClient::UFightingVROnlineSessionClient()
{
}

void UFightingVROnlineSessionClient::OnSessionUserInviteAccepted(
	const bool							bWasSuccess,
	const int32							ControllerId,
	TSharedPtr< const FUniqueNetId > 	UserId,
	const FOnlineSessionSearchResult &	InviteResult
)
{
	UE_LOG(LogOnline, Verbose, TEXT("HandleSessionUserInviteAccepted: bSuccess: %d, ControllerId: %d, User: %s"), bWasSuccess, ControllerId, UserId.IsValid() ? *UserId->ToString() : TEXT("NULL"));

	if (!bWasSuccess)
	{
		return;
	}

	if (!InviteResult.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no search result."));
		return;
	}

	if (!UserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no user."));
		return;
	}

	UFightingVRInstance* FightingVRInstance = Cast<UFightingVRInstance>(GetGameInstance());

	if (FightingVRInstance)
	{
		FFightingVRPendingInvite PendingInvite;

		// Set the pending invite, and then go to the initial screen, which is where we will process it
		PendingInvite.ControllerId = ControllerId;
		PendingInvite.UserId = UserId;
		PendingInvite.InviteResult = InviteResult;
		PendingInvite.bPrivilegesCheckedAndAllowed = false;

		FightingVRInstance->SetPendingInvite(PendingInvite);
		FightingVRInstance->GotoState(FightingVRInstanceState::PendingInvite);
	}
}