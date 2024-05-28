// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSessionClient.h"
#include "FightingVROnlineSessionClient.generated.h"

UCLASS(Config = Game)
class UFightingVROnlineSessionClient : public UOnlineSessionClient
{
	GENERATED_BODY()

public:
	/** Ctor */
	UFightingVROnlineSessionClient();

	virtual void OnSessionUserInviteAccepted(
		const bool							bWasSuccess,
		const int32							ControllerId,
		TSharedPtr< const FUniqueNetId >	UserId,
		const FOnlineSessionSearchResult &	InviteResult
	) override;

};
