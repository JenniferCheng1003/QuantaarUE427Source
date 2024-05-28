// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRCheatManager.h"
#include "FightingVR.h"
#include "Online/FightingVRPlayerState.h"
#include "Bots/FightingVRAIController.h"

UFightingVRCheatManager::UFightingVRCheatManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UFightingVRCheatManager::ToggleInfiniteAmmo()
{
	AFightingVRPlayerController* MyPC = GetOuterAFightingVRPlayerController();

	MyPC->SetInfiniteAmmo(!MyPC->HasInfiniteAmmo());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite ammo: %s"), MyPC->HasInfiniteAmmo() ? TEXT("ENABLED") : TEXT("off")));
}

void UFightingVRCheatManager::ToggleInfiniteClip()
{
	AFightingVRPlayerController* MyPC = GetOuterAFightingVRPlayerController();

	MyPC->SetInfiniteClip(!MyPC->HasInfiniteClip());
	MyPC->ClientMessage(FString::Printf(TEXT("Infinite clip: %s"), MyPC->HasInfiniteClip() ? TEXT("ENABLED") : TEXT("off")));
}

void UFightingVRCheatManager::ToggleMatchTimer()
{
	AFightingVRPlayerController* MyPC = GetOuterAFightingVRPlayerController();

	AFightingVRState* const MyGameState = MyPC->GetWorld()->GetGameState<AFightingVRState>();
	if (MyGameState && MyGameState->GetLocalRole() == ROLE_Authority)
	{
		MyGameState->bTimerPaused = !MyGameState->bTimerPaused;
		MyPC->ClientMessage(FString::Printf(TEXT("Match timer: %s"), MyGameState->bTimerPaused ? TEXT("PAUSED") : TEXT("running")));
	}
}

void UFightingVRCheatManager::ForceMatchStart()
{
	AFightingVRPlayerController* const MyPC = GetOuterAFightingVRPlayerController();

	AFightingVRMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AFightingVRMode>();
	if (MyGame && MyGame->GetMatchState() == MatchState::WaitingToStart)
	{
		MyGame->StartMatch();
	}
}

void UFightingVRCheatManager::ChangeTeam(int32 NewTeamNumber)
{
	AFightingVRPlayerController* MyPC = GetOuterAFightingVRPlayerController();

	AFightingVRPlayerState* MyPlayerState = Cast<AFightingVRPlayerState>(MyPC->PlayerState);
	if (MyPlayerState && MyPlayerState->GetLocalRole() == ROLE_Authority)
	{
		MyPlayerState->SetTeamNum(NewTeamNumber);
		MyPC->ClientMessage(FString::Printf(TEXT("Team changed to: %d"), MyPlayerState->GetTeamNum()));
	}
}

void UFightingVRCheatManager::Cheat(const FString& Msg)
{
	GetOuterAFightingVRPlayerController()->ServerCheat(Msg.Left(128));
}

void UFightingVRCheatManager::SpawnBot()
{
	AFightingVRPlayerController* const MyPC = GetOuterAFightingVRPlayerController();
	APawn* const MyPawn = MyPC->GetPawn();
	AFightingVRMode* const MyGame = MyPC->GetWorld()->GetAuthGameMode<AFightingVRMode>();
	UWorld* World = MyPC->GetWorld();
	if (MyPawn && MyGame && World)
	{
		static int32 CheatBotNum = 50;
		AFightingVRAIController* FightingVRAIController = MyGame->CreateBot(CheatBotNum++);
		MyGame->RestartPlayer(FightingVRAIController);		
	}
}