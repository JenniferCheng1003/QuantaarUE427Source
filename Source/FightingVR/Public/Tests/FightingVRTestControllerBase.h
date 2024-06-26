// Copyright Epic Games, Inc.All Rights Reserved.
#pragma once

#include "GauntletTestController.h"
#include "FightingVRInstance.h"
#include "SharedPointer.h"
#include "OnlineSessionSettings.h"
#include "FightingVRTestControllerBase.generated.h"

// Based on LOGIN_REQUIRED_FOR_ONLINE_PLAY in FightingVRMainMenu.cpp
#if PLATFORM_SWITCH
#define LOGIN_REQUIRED_FOR_ONLINE_PLAY 1
#else
#define LOGIN_REQUIRED_FOR_ONLINE_PLAY 0
#endif

UCLASS()
class UFightingVRTestControllerBase : public UGauntletTestController, public TSharedFromThis<UFightingVRTestControllerBase>
{
	GENERATED_BODY()

public:
	virtual void OnInit() override;
	virtual void OnPostMapChange(UWorld* World) override;

protected:
	// Login
	uint8 bIsLoggedIn : 1;
	uint8 bIsLoggingIn : 1;
	FDelegateHandle OnLoginCompleteDelegateHandle;

	// Quick Match
	uint8 bInQuickMatchSearch : 1;
	uint8 bFoundQuickMatchGame : 1;
	TSharedPtr<FOnlineSessionSearch> QuickMatchSearchSettings;

	// Game Search
	uint8 bIsSearchingForGame : 1;
	uint8 bFoundGame : 1;

	// Match Cycling 
	uint8 NumOfCycledMatches;
	uint8 TargetNumOfCycledMatches;

	virtual void OnTick(float TimeDelta) override;

	// Login
	virtual void StartPlayerLoginProcess();
	virtual void OnLoginUIClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const FOnlineError& Error = FOnlineError());
	virtual void OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults);

	virtual void StartLoginTask();
	virtual void OnLoginTaskComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	virtual void StartOnlinePrivilegeTask();
	virtual void OnUserCanPlayOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults);

	void CheckApplicationLicenseValid();

	// Host Game
	virtual void HostGame();

	// Quick Match
	virtual void StartQuickMatch();
	void OnMatchmakingComplete(FName SessionName, const FOnlineError& ErrorDetails, const FSessionMatchmakingResults& Results);

	// Game Search
	virtual void StartSearchingForGame();
	virtual void UpdateSearchStatus();

	// Helper Functions
	virtual UFightingVRInstance* GetGameInstance() const;
	virtual const FName GetGameInstanceState() const;
	virtual AFightingVRSession* GetGameSession() const;
	virtual bool IsInGame() const;
	virtual ULocalPlayer* GetFirstLocalPlayer() const;
};
