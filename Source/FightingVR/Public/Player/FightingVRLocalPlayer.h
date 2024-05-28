// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRPersistentUser.h"
#include "FightingVRLocalPlayer.generated.h"

UCLASS(config=Engine, transient)
class UFightingVRLocalPlayer : public ULocalPlayer
{
	GENERATED_UCLASS_BODY()

public:

	virtual void SetControllerId(int32 NewControllerId) override;

	virtual FString GetNickname() const;

	class UFightingVRPersistentUser* GetPersistentUser() const;
	
	/** Initializes the PersistentUser */
	void LoadPersistentUser();

private:
	/** Persistent user data stored between sessions (i.e. the user's savegame) */
	UPROPERTY()
	class UFightingVRPersistentUser* PersistentUser;
};



