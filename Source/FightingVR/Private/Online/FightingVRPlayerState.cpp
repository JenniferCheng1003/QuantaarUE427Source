// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRPlayerState.h"
#include "FightingVR.h"
#include "Net/OnlineEngineInterface.h"

AFightingVRPlayerState::AFightingVRPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TeamNumber = 0;
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void AFightingVRPlayerState::Reset()
{
	Super::Reset();
	
	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void AFightingVRPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	if (UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::RegisterPlayerWithSession(bWasFromInvite);
	}
}

void AFightingVRPlayerState::UnregisterPlayerWithSession()
{
	if (!IsFromPreviousLevel() && UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::UnregisterPlayerWithSession();
	}
}

void AFightingVRPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColors();
}

void AFightingVRPlayerState::SetTeamNum(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;

	UpdateTeamColors();
}

void AFightingVRPlayerState::OnRep_TeamColor()
{
	UpdateTeamColors();
}

void AFightingVRPlayerState::AddBulletsFired(int32 NumBullets)
{
	NumBulletsFired += NumBullets;
}

void AFightingVRPlayerState::AddRocketsFired(int32 NumRockets)
{
	NumRocketsFired += NumRockets;
}

void AFightingVRPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void AFightingVRPlayerState::SetMatchId(const FString& CurrentMatchId)
{
	MatchId = CurrentMatchId;
}

void AFightingVRPlayerState::CopyProperties(APlayerState* PlayerState)
{	
	Super::CopyProperties(PlayerState);

	AFightingVRPlayerState* FightingVRPlayer = Cast<AFightingVRPlayerState>(PlayerState);
	if (FightingVRPlayer)
	{
		FightingVRPlayer->TeamNumber = TeamNumber;
	}	
}

void AFightingVRPlayerState::UpdateTeamColors()
{
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != NULL)
	{
		AFightingVRCharacter* FightingVRCharacter = Cast<AFightingVRCharacter>(OwnerController->GetCharacter());
		if (FightingVRCharacter != NULL)
		{
			FightingVRCharacter->UpdateTeamColorsAllMIDs();
		}
	}
}

int32 AFightingVRPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 AFightingVRPlayerState::GetKills() const
{
	return NumKills;
}

int32 AFightingVRPlayerState::GetDeaths() const
{
	return NumDeaths;
}

int32 AFightingVRPlayerState::GetNumBulletsFired() const
{
	return NumBulletsFired;
}

int32 AFightingVRPlayerState::GetNumRocketsFired() const
{
	return NumRocketsFired;
}

bool AFightingVRPlayerState::IsQuitter() const
{
	return bQuitter;
}

FString AFightingVRPlayerState::GetMatchId() const
{
	return MatchId;
}

void AFightingVRPlayerState::ScoreKill(AFightingVRPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void AFightingVRPlayerState::ScoreDeath(AFightingVRPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void AFightingVRPlayerState::ScorePoints(int32 Points)
{
	AFightingVRState* const MyGameState = GetWorld()->GetGameState<AFightingVRState>();
	if (MyGameState && TeamNumber >= 0)
	{
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}

	SetScore(GetScore() + Points);
}

void AFightingVRPlayerState::InformAboutKill_Implementation(class AFightingVRPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AFightingVRPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->GetUniqueId().IsValid())
	{	
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{		
			AFightingVRPlayerController* TestPC = Cast<AFightingVRPlayerController>(*It);
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				FUniqueNetIdRepl LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() &&  *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->GetUniqueId())
				{			
					TestPC->OnKill();
				}
			}
		}
	}
}

void AFightingVRPlayerState::BroadcastDeath_Implementation(class AFightingVRPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AFightingVRPlayerState* KilledPlayerState)
{	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		AFightingVRPlayerController* TestPC = Cast<AFightingVRPlayerController>(*It);
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);				
		}
	}	
}

void AFightingVRPlayerState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AFightingVRPlayerState, TeamNumber );
	DOREPLIFETIME( AFightingVRPlayerState, NumKills );
	DOREPLIFETIME( AFightingVRPlayerState, NumDeaths );
}

FString AFightingVRPlayerState::GetShortPlayerName() const
{
	if( GetPlayerName().Len() > MAX_PLAYER_NAME_LENGTH )
	{
		return GetPlayerName().Left(MAX_PLAYER_NAME_LENGTH) + "...";
	}
	return GetPlayerName();
}