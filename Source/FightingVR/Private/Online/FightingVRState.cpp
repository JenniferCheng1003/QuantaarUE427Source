// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRState.h"
#include "FightingVR.h"
#include "Online/FightingVRPlayerState.h"
#include "FightingVRInstance.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineGameMatchesInterface.h"

AFightingVRState::AFightingVRState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	RemainingTime = 0;
	bTimerPaused = false;

	UFightingVRInstance* GameInstance = GetWorld() != nullptr ? Cast<UFightingVRInstance>(GetWorld()->GetGameInstance()) : nullptr;

	GameMatches.Initialize(this, GameInstance);
}

void AFightingVRState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AFightingVRState, NumTeams );
	DOREPLIFETIME( AFightingVRState, RemainingTime );
	DOREPLIFETIME( AFightingVRState, bTimerPaused );
	DOREPLIFETIME( AFightingVRState, TeamScores );
}

void AFightingVRState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, AFightingVRPlayerState*> SortedMap;
	for(int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		AFightingVRPlayerState* CurPlayerState = Cast<AFightingVRPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNum() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->GetScore()), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for(TMultiMap<int32, AFightingVRPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}
	
}


void AFightingVRState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AFightingVRMode* const GameMode = Cast<AFightingVRMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		UFightingVRInstance* GameInstance = Cast<UFightingVRInstance>(GetGameInstance());
		if (GameInstance)
		{
			GameInstance->RemoveSplitScreenPlayers();
		}

		AFightingVRPlayerController* const PrimaryPC = Cast<AFightingVRPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			PrimaryPC->HandleReturnToMainMenu();
		}
	}
}

void AFightingVRState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	GameMatches.HandleMatchHasStarted(ActivityId, NumTeams);
}

void AFightingVRState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	GameMatches.HandleMatchHasEnded(bEnableGameFeedback, NumTeams, MakeArrayView(TeamScores));
}