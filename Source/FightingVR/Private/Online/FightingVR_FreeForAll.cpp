// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVR_FreeForAll.h"
#include "FightingVR.h"
#include "FightingVRPlayerState.h"

AFightingVR_FreeForAll::AFightingVR_FreeForAll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDelayedStart = true;
}

void AFightingVR_FreeForAll::DetermineMatchWinner()
{
	AFightingVRState const* const MyGameState = CastChecked<AFightingVRState>(GameState);
	float BestScore = MIN_flt;
	int32 BestPlayer = -1;
	int32 NumBestPlayers = 0;

	for (int32 i = 0; i < MyGameState->PlayerArray.Num(); i++)
	{
		const float PlayerScore = MyGameState->PlayerArray[i]->GetScore();
		if (BestScore < PlayerScore)
		{
			BestScore = PlayerScore;
			BestPlayer = i;
			NumBestPlayers = 1;
		}
		else if (BestScore == PlayerScore)
		{
			NumBestPlayers++;
		}
	}

	WinnerPlayerState = (NumBestPlayers == 1) ? Cast<AFightingVRPlayerState>(MyGameState->PlayerArray[BestPlayer]) : NULL;
}

bool AFightingVR_FreeForAll::IsWinner(AFightingVRPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState == WinnerPlayerState;
}
