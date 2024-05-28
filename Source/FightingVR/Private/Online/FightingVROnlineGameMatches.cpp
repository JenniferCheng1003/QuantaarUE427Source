// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVROnlineGameMatches.h"
#include "OnlineSubsystemUtils.h"
#include "FightingVRInstance.h"
#include "FightingVRPlayerState.h"

void FFightingVROnlineGameMatches::Initialize(AFightingVRState* InGameState, UFightingVRInstance* InGameInstance)
{
	if (InGameInstance != nullptr)
	{
		GameInstance = InGameInstance;
	}

	if (InGameState != nullptr)
	{
		GameState = InGameState;
	}
}
void FFightingVROnlineGameMatches::CreateMatch(const FUniqueNetId& LocalOwnerId, const FString& ActivitId, const TArray<FGameMatchPlayer>& Players, const TArray<FGameMatchTeam>& Teams)
{
	if (IOnlineGameMatchesPtr MatchesInterface = IOnlineSubsystem::Get()->GetGameMatchesInterface())
	{
		FGameMatchesData MatchData;
		FGameMatchRoster Roaster;
		Roaster.Players = Players;
		Roaster.Teams = Teams;
		MatchData.MatchesRoster = Roaster;
		MatchData.ActivityId = ActivitId;

		TWeakObjectPtr<AFightingVRState> WeakGameState(GameState);
		FOnCreateGameMatchComplete CreateMatchCompleteDelegate = FOnCreateGameMatchComplete::CreateLambda(
			[this, WeakGameState](const FUniqueNetId& LambdaLocalUserId, const FString& LambdaMatchId, const FOnlineError& LambdaResult)
		{
			if (WeakGameState.IsValid())
			{
				OnCreateMatchComplete(LambdaLocalUserId, LambdaMatchId, LambdaResult);
			}
		});
		MatchesInterface->CreateGameMatch(LocalOwnerId, MatchData, CreateMatchCompleteDelegate);
	}
	else
	{
		// No valid matches interface
		return;
	}
}

void FFightingVROnlineGameMatches::OnCreateMatchComplete(const FUniqueNetId& LocalUserId, const FString& MatchId, const FOnlineError& Result)
{
	if (Result.WasSuccessful())
	{
		if (IOnlineSessionPtr Sessions = IOnlineSubsystem::Get()->GetSessionInterface())
		{
			FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession);
			if (NamedSession != nullptr)
			{
				NamedSession->SessionSettings.Set(TEXT("SETTING_MATCHID"), MatchId);
				Sessions->UpdateSession(NAME_GameSession, NamedSession->SessionSettings, true);

				StartMatch(LocalUserId, MatchId);
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Namedsession is invalid"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::OnCreateMatchComplete: Create game match failed: [%s]"), *Result.ErrorCode);
	}
}

void FFightingVROnlineGameMatches::StartMatch(const FUniqueNetId& LocalUserId, const FString& MatchId)
{
	if (IOnlineGameMatchesPtr Matches = IOnlineSubsystem::Get()->GetGameMatchesInterface())
	{
		TWeakObjectPtr<AFightingVRState> WeakGameState(GameState);
		FOnGameMatchStatusUpdateComplete MatchStatusUpdateCompleteDelegate = FOnGameMatchStatusUpdateComplete::CreateLambda(
			[this, WeakGameState](const FUniqueNetId& LambdaLocalUserId, const EUpdateGameMatchStatus& LambdaStatus, const FOnlineError& LambdaResult)
		{
			if (WeakGameState.IsValid())
			{
				OnMatchStatusUpdateComplete(LambdaLocalUserId, LambdaStatus, LambdaResult);
			}
		});

		Matches->UpdateGameMatchStatus(LocalUserId, MatchId, EUpdateGameMatchStatus::InProgress, MatchStatusUpdateCompleteDelegate);
	}
	else
	{
		// No valid matches interface
		return;
	}
}

void FFightingVROnlineGameMatches::OnMatchStatusUpdateComplete(const FUniqueNetId& LocalUserId, const EUpdateGameMatchStatus& Status, const FOnlineError& Result)
{
	if (Result.WasSuccessful())
	{
		UE_LOG(LogOnline, Log, TEXT("Match status updated to [%d]"), Status);
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::OnMatchStatusUpdateComplete: Could not set new status: [%s]"), *Result.ToLogString());
	}
}

void FFightingVROnlineGameMatches::EndMatch(const FUniqueNetId& LocalOwnerId, const FString& MatchId, const FFinalGameMatchReport& FinalReport)
{
	if(IOnlineGameMatchesPtr MatchesInterface = IOnlineSubsystem::Get()->GetGameMatchesInterface())
	{
		TWeakObjectPtr<AFightingVRState> WeakGameState(GameState);
		FOnGameMatchReportComplete MatchReportCompleteDelegate = FOnGameMatchReportComplete::CreateLambda(
			[this, WeakGameState, MatchId, FinalReport](const FUniqueNetId& LambdaLocalUserId, const FOnlineError& LambdaResult)
		{
			if (WeakGameState.IsValid())
			{
				OnEndMatchComplete(LambdaLocalUserId, LambdaResult, MatchId, FinalReport.bLeaveGameFeedback);
			}
		});

		MatchesInterface->ReportGameMatchResults(LocalOwnerId, MatchId, FinalReport, MatchReportCompleteDelegate);
	}
	else
	{
		// No valid matches interface
		return;
	}
}

void FFightingVROnlineGameMatches::OnEndMatchComplete(const FUniqueNetId& LocalUserId, const FOnlineError& Result, FString MatchId, bool bRequestReview)
{
	if (Result.WasSuccessful())
	{
		UE_LOG(LogOnlineGame, Log, TEXT("FFightingVROnlineGameMatches::OnEndMatchComplete: Report end of game match [%s] was successful"), *MatchId);
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("FFightingVROnlineGameMatches::OnEndMatchComplete: Report end of game match failed: [%s]"), *Result.ErrorCode);
	}
}

void FFightingVROnlineGameMatches::LeaveGameMatchFeedback(const FUniqueNetId& LocalUserId, const FString& MatchId, const bool bRequestReview)
{
	if (bRequestReview)
	{
		if (IOnlineGameMatchesPtr Matches = IOnlineSubsystem::Get()->GetGameMatchesInterface())
		{
			bool bTeamReviewOnly = true;

			TWeakObjectPtr<AFightingVRState> WeakGameState(GameState);
			FOnGameMatchFeedbackComplete GameMatchFeedbackCompleteDelegate = FOnGameMatchFeedbackComplete::CreateLambda(
				[this, WeakGameState](const FUniqueNetId& LambdaLocalUserId, const FOnlineError& LambdaResult)
			{
				if (WeakGameState.IsValid())
				{
					OnLeaveGameMatchFeedbackComplete(LambdaLocalUserId, LambdaResult);
				}
			});

			Matches->ProvideGameMatchFeedback(LocalUserId, MatchId, bTeamReviewOnly, GameMatchFeedbackCompleteDelegate);
		}
		else
		{
			// No valid matches interface
			return;
		}
	}
}

void FFightingVROnlineGameMatches::OnLeaveGameMatchFeedbackComplete(const FUniqueNetId& LocalUserId, const FOnlineError& Result)
{
	if (Result.WasSuccessful())
	{
		UE_LOG(LogOnline, Log, TEXT("FFightingVROnlineGameMatches::OnLeaveGameMatchFeedbackComplete: Feedback results process complete for [%s]"), *LocalUserId.ToDebugString());
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::OnLeaveGameMatchFeedbackComplete: Feedback results process failed for [%s]: [%s]"), *LocalUserId.ToDebugString(), *Result.ErrorCode);
	}
}

void FFightingVROnlineGameMatches::BuildTeamPlayerGameMatchStats(const TSharedRef<const FUniqueNetId> PlayerNetId, const int32 TeamId, const FString& StatKey, const FString& StatValue)
{
	FGameMatchPlayerStats PlayerStat;
	PlayerStat.PlayerId = PlayerNetId;

	FGameMatchStatsData MatchStat;
	MatchStat.StatsKey = StatKey;
	MatchStat.StatsValue = StatValue;
	PlayerStat.PlayerStats.Emplace(MatchStat);
	TArray<FGameMatchPlayerStats>* PlayerStatsArray = TeamToPlayerStatsMap.Find(TeamId);
	if (PlayerStatsArray != nullptr)
	{
		PlayerStatsArray->Emplace(PlayerStat);
	}
	else
	{
		TArray<FGameMatchPlayerStats> PlayerStats;
		PlayerStats.Emplace(PlayerStat);
		TeamToPlayerStatsMap.Emplace(TeamId, PlayerStats);
	}
}

void FFightingVROnlineGameMatches::BuildTeamGameMatchStats(TArray<FGameMatchTeamStats>& TeamStats)
{
	TeamStats.Reserve(TeamToPlayerStatsMap.Num());
	for (const TPair<int32, TArray<FGameMatchPlayerStats>>& Team : TeamToPlayerStatsMap)
	{
		FGameMatchTeamStats TeamStat;
		TeamStat.TeamId = FString::FromInt(Team.Key);
		TeamStat.TeamMemberStats = Team.Value;
		TeamStats.Emplace(TeamStat);
	}
}

void FFightingVROnlineGameMatches::BuildPlayerGameMatchResults(const TSharedRef<const FUniqueNetId> PlayerNetId, const int32 TeamId, FGameMatchPlayerResult PlayerResult)
{

	TArray<FGameMatchPlayerResult>* PlayerResultArray = TeamPlayerResultsMap.Find(TeamId);
	if (PlayerResultArray != nullptr)
	{
		PlayerResultArray->Emplace(PlayerResult);
	}
	else
	{
		TArray<FGameMatchPlayerResult> MemberResult;
		MemberResult.Emplace(PlayerResult);
		TeamPlayerResultsMap.Emplace(TeamId, MemberResult);
	}
}

void FFightingVROnlineGameMatches::BuildTeamGameMatchResults(const TArrayView<int32> TeamScores, const int32& NumTeams, TArray<FGameMatchTeamResult>& TeamResults)
{
	// Determine team rank
	int32 BestScore = MIN_uint32;
	
	for (int32 TeamIndex = 0; TeamIndex < TeamScores.Num(); ++TeamIndex)
	{
		const int32 TeamScore = TeamScores[TeamIndex];
		if (TeamScore > BestScore)
		{
			BestScore = TeamScore;
		}
	}

	// Create the team structure with rank and score
	for (int32 TeamIndex = 0; TeamIndex < NumTeams; ++TeamIndex)
	{
		FGameMatchTeamResult TeamResult;
		if (TeamScores.Num() > 0)
		{
			TeamResult.Score = TeamScores[TeamIndex];
			TeamResult.Rank = (TeamResult.Score == BestScore) ? 1 : 2;
		}
		else
		{
			TeamResult.Score = 0;
			TeamResult.Rank = 0;
		}

		TeamResult.TeamId = FString::FromInt(TeamIndex);

		TArray<FGameMatchPlayerResult>* MemberResults = TeamPlayerResultsMap.Find(TeamIndex);
		if (MemberResults != nullptr)
		{
			TeamResult.MembersResult = *MemberResults;
		}

		TeamResults.Emplace(TeamResult);
	}
}

bool FFightingVROnlineGameMatches::GetMatchOwnerAndId(TSharedPtr<const FUniqueNetId>& OwnerId, FString& MatchId)
{
	FNamedOnlineSession* NamedSession;
	if (GetNamedSessionInfo(NamedSession))
	{
		TSharedPtr<const FUniqueNetId> OwningUserId = NamedSession->OwningUserId;
		if (OwningUserId.IsValid())
		{
			OwnerId = OwningUserId;
			NamedSession->SessionSettings.Get(TEXT("SETTING_MATCHID"), MatchId);

			if (MatchId.IsEmpty())
			{
				UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchOwnerAndId: MatchId was not found."));
				return false;
			}
			return true;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchOwnerAndId: OwningUserId is invalid"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchOwnerAndId: GetNamedSessionInfo failed"));
	}

	return false;
}

bool FFightingVROnlineGameMatches::GetNamedSessionInfo(FNamedOnlineSession*& NamedSession)
{
	if (IOnlineSessionPtr Sessions = IOnlineSubsystem::Get()->GetSessionInterface())
	{
		NamedSession = Sessions->GetNamedSession(NAME_GameSession);
		if (NamedSession != nullptr)
		{
			return true;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetNamedSessionInfo: Namedsession is invalid"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetNamedSessionInfo: session interface is invalid"));
	}

	return false;
}

TSharedPtr<const FUniqueNetId> FFightingVROnlineGameMatches::GetMatchOwnerId()
{
	TSharedPtr<const FUniqueNetId> OwnerId;
	FNamedOnlineSession* NamedSession;
	if (GetNamedSessionInfo(NamedSession))
	{
		TSharedPtr<const FUniqueNetId> OwningUserId = NamedSession->OwningUserId;
		if (OwningUserId.IsValid())
		{
			OwnerId = OwningUserId;
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchOwnerId: OwningUserId is invalid"));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchOwnerId: GetNamedSessionInfo failed"));
	}

	return OwnerId;
}

bool FFightingVROnlineGameMatches::GetMatchId(FString& MatchId)
{
	FNamedOnlineSession* NamedSession;
	if (GetNamedSessionInfo(NamedSession))
	{
		NamedSession->SessionSettings.Get(TEXT("SETTING_MATCHID"), MatchId);

		if (MatchId.IsEmpty())
		{
			UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchId: MatchId was not found."));
			return false;
		}
		return true;
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::GetMatchId: GetNamedSessionInfo failed"));
	}

	return false;
}

bool FFightingVROnlineGameMatches::IsClientRepresentative(const TSharedPtr<const FUniqueNetId>& RepresentativeId)
{
	TWeakObjectPtr<UFightingVRInstance> WeakGameInstance(GameInstance);
	if (WeakGameInstance.IsValid())
	{
		const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			FUniqueNetIdRepl LocalPlayer = LocalPlayers[i]->GetUniqueNetIdFromCachedControllerId();
			if (LocalPlayer != nullptr)
			{
				if (*RepresentativeId == *LocalPlayer)
				{
					UE_LOG(LogOnline, Verbose, TEXT("LocalOwnerId [%s] is our representative"), *RepresentativeId->ToDebugString());
					return true;
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::IsClientRepresentative: LocalPlayer is not valid"));
			}
		}
	}
	else
	{
		// Game instance is not valid
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::IsClientRepresentative: Game instance is not valid"));
	}
	return false;
}

void FFightingVROnlineGameMatches::HandleMatchHasStarted(const FString& ActivityId, int32& NumTeams)
{
	IOnlineGameMatchesPtr Matches = IOnlineSubsystem::Get()->GetGameMatchesInterface();
	if(!Matches.IsValid())
	{
		// No valid matches interface
		return;
	}

	TWeakObjectPtr<UFightingVRInstance> WeakGameInstance(GameInstance);
	if (!WeakGameInstance.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::HandleMatchHasStarted: Game instance is invalid"));
		return;
	}

	TSharedPtr<const FUniqueNetId> OwnerId = GetMatchOwnerId();
	if (OwnerId.IsValid())
	{
		if (IsClientRepresentative(OwnerId))
		{
			TArray<FGameMatchPlayer> Players;
			//AccountId, TeamId Map
			TMap<FString, TArray<TSharedRef<const FUniqueNetId>>> TeamMembershipMap;
			int32 PlayerIndex = 0;
			// Iterate all players to build out the match
			for (TActorIterator<AFightingVRPlayerState> It(WeakGameInstance->GetWorld()); It; ++It)
			{
				bool bIsABot = (*It)->IsABot();

				const TSharedPtr<const FUniqueNetId>& PlayerId = (*It)->GetUniqueId().GetUniqueNetId();
				if (!PlayerId.IsValid() && !bIsABot)
				{
					UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::HandleMatchHasStarted: Player is invalid"));
					return;
				}
				TSharedPtr<const FUniqueNetId> PlayerNetId = bIsABot ? MakeShared<FUniqueNetIdString>(FString::FromInt((*It)->GetPlayerId())) : PlayerId;
				FGameMatchPlayer Player;
				Player.bIsNpc = bIsABot;
				FString PlayerName((*It)->GetPlayerName());
				Player.PlayerName = PlayerName;
				Player.PlayerId = PlayerNetId;
				Players.Emplace(Player);

				//Save our id for after the match is over.
				FString PlayerIdString(FString::FromInt((*It)->GetPlayerId()));
				PlayerIdToNetIdIndexMap.Emplace(PlayerIdString, PlayerIndex);
				PlayerNetIds.Emplace(PlayerNetId.ToSharedRef());
				PlayerIndex++;

				FString TeamId = FString::FromInt((*It)->GetTeamNum());

				//Build out the team membership mapping
				TArray<TSharedRef<const FUniqueNetId>>& TeamMemberArray = TeamMembershipMap.FindOrAdd(TeamId);
				TeamMemberArray.Emplace(PlayerNetId.ToSharedRef());
			}

			NumTeams = TeamMembershipMap.Num();

			TArray<FGameMatchTeam> Teams;
			for (const TPair<FString, TArray<TSharedRef<const FUniqueNetId>>>& CurrentTeam : TeamMembershipMap)
			{
				FGameMatchTeam Team;
				Team.TeamId = CurrentTeam.Key;
				Team.TeamMemberIds = CurrentTeam.Value;
				Teams.Emplace(Team);
			}

			if (ActivityId.IsEmpty())
			{
				// An activity Id must be present in game ini or we should fail.
				UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::HandleMatchHasStarted: Activity id is not set.  Please add it to the game ini"));
				return;
			}

			CreateMatch(*OwnerId, ActivityId, Players, Teams);
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("FFightingVROnlineGameMatches::HandleMatchHasStarted: Failed to get a valid session owner"));
	}
}

void FFightingVROnlineGameMatches::HandleMatchHasEnded(const bool bEnableGameFeedback, const int32 NumTeams, TArrayView<int32> TeamScores)
{
	IOnlineGameMatchesPtr Matches = IOnlineSubsystem::Get()->GetGameMatchesInterface();
	if (!Matches.IsValid())
	{
		// No valid matches interface
		return;
	}

	TSharedPtr<const FUniqueNetId> OwnerId;
	FString MatchId;

	if (GetMatchOwnerAndId(OwnerId, MatchId))
	{
		if (IsClientRepresentative(OwnerId))
		{
			TArray<FGameMatchTeamResult> TeamsResult;

			if (NumTeams > 0)
			{

				TWeakObjectPtr<AFightingVRState> WeakGameState(GameState);
				if (!WeakGameState.IsValid())
				{
					UE_LOG(LogOnline, Warning, TEXT("GameState is not valid"));
					return;
				}
				TArray<RankedPlayerMap> TeamRankedPlayers;
				TeamRankedPlayers.AddZeroed(NumTeams);
				for (int32 TeamIndex = 0; TeamIndex < NumTeams; ++TeamIndex)
				{
					WeakGameState->GetRankedMap(TeamIndex, TeamRankedPlayers[TeamIndex]);
				}

				for (int32 TeamIndex = 0; TeamIndex < TeamRankedPlayers.Num(); ++TeamIndex)
				{

					for (const TPair<int32, TWeakObjectPtr<AFightingVRPlayerState>>& RankedPlayer : TeamRankedPlayers[TeamIndex])
					{

						FString PlayerIdString(FString::FromInt((*RankedPlayer.Value).GetPlayerId()));
						const int32* NetIdIndex = PlayerIdToNetIdIndexMap.Find(PlayerIdString);
						if (NetIdIndex == nullptr)
						{
							UE_LOG(LogOnline, Warning, TEXT("AFightingVRState::HandleMatchHasEnded: Could not find player net id index in map: [%s]"), *PlayerIdString);
							return;
						}

						const TSharedRef<const FUniqueNetId> PlayerNetId = PlayerNetIds[*NetIdIndex];

						FGameMatchPlayerResult PlayerResult;
						PlayerResult.PlayerId = PlayerNetId;
						PlayerResult.Rank = RankedPlayer.Key;
						PlayerResult.Score = (*RankedPlayer.Value).GetScore();

						int32 TeamId = (*RankedPlayer.Value).GetTeamNum();

						BuildPlayerGameMatchResults(PlayerNetId, TeamId, PlayerResult);

						// Setup match stats for deaths and kills
						BuildTeamPlayerGameMatchStats(PlayerNetId, TeamId, TEXT("Deaths"), FString::FromInt((*RankedPlayer.Value).GetDeaths()));
						BuildTeamPlayerGameMatchStats(PlayerNetId, TeamId, TEXT("Kills"), FString::FromInt((*RankedPlayer.Value).GetKills()));

						// Set the match id on the player for UI access
						(*RankedPlayer.Value).SetMatchId(MatchId);

					}
				}

				//Now split out by teams.
				TArray<FGameMatchTeamResult> TeamResults;
				BuildTeamGameMatchResults(TeamScores, NumTeams, TeamResults);

				FGameMatchCompetitiveResults CompetitiveMatch;
				CompetitiveMatch.TeamsResult = TeamResults;

				FGameMatchResult Results;
				Results.CompetitiveResult = CompetitiveMatch;

				// Build the final match structure
				FFinalGameMatchReport MatchReport;
				MatchReport.Results = Results;

				// If we have extra stats recorded we need to provide them as well
				TArray<FGameMatchTeamStats> TeamStats;
				BuildTeamGameMatchStats(TeamStats);

				if (TeamStats.Num() > 0)
				{
					FGameMatchStats MatchStats;
					MatchStats.TeamStats = TeamStats;
					MatchReport.Stats = MatchStats;
				}

				MatchReport.bLeaveGameFeedback = bEnableGameFeedback;
				MatchReport.MatchType = GAME_MATCH_TYPE_COMPETITIVE;
				MatchReport.GroupType = EMatchGroupType::Teams;

				EndMatch(*OwnerId, MatchId, MatchReport);
			}
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Failed to get named session info"));
		return;
	}
}