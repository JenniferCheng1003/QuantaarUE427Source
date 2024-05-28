// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "FightingVRTypes.h"
#include "OnlineLeaderboardInterface.h"

// these are normally exported from platform-specific tools
#define LEADERBOARD_STAT_SCORE			"FightingVRAllTimeMatchResultsScore"
#define LEADERBOARD_STAT_KILLS			"FightingVRAllTimeMatchResultsFrags"
#define LEADERBOARD_STAT_DEATHS			"FightingVRAllTimeMatchResultsDeaths"
#define LEADERBOARD_STAT_MATCHESPLAYED	"FightingVRAllTimeMatchResultsMatchesPlayed"

/**
 *	'AllTime' leaderboard read object
 */
class FFightingVRAllTimeMatchResultsRead : public FOnlineLeaderboardRead
{
public:

	FFightingVRAllTimeMatchResultsRead()
	{
		// Default properties
		LeaderboardName = FName(TEXT("FightingVRAllTimeMatchResults"));
		SortedColumn = LEADERBOARD_STAT_SCORE;

		// Define default columns
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_SCORE, EOnlineKeyValuePairDataType::Int32);
	}
};

/**
 *	'AllTime' leaderboard write object
 */
class FFightingVRAllTimeMatchResultsWrite : public FOnlineLeaderboardWrite
{
public:

	FFightingVRAllTimeMatchResultsWrite()
	{
		// Default properties
		new (LeaderboardNames) FName(TEXT("FightingVRAllTimeMatchResults"));
		RatedStat = LEADERBOARD_STAT_SCORE;
		DisplayFormat = ELeaderboardFormat::Number;
		SortMethod = ELeaderboardSort::Descending;
		UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
	}
};

