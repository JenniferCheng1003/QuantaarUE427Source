// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * General session settings for a FightingVR game
 */
class FFightingVROnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FFightingVROnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FFightingVROnlineSessionSettings() {}
};

/**
 * General search setting for a FightingVR game
 */
class FFightingVROnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FFightingVROnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FFightingVROnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FFightingVROnlineSearchSettingsEmptyDedicated : public FFightingVROnlineSearchSettings
{
public:
	FFightingVROnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FFightingVROnlineSearchSettingsEmptyDedicated() {}
};
