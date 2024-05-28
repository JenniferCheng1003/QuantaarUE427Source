// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRHUDPCTrackerBase.h"
#include "FightingVR.h"


/** Initialize with a world context. */
void FightingVRHUDPCTrackerBase::Init( const FLocalPlayerContext& InContext )
{
	Context = InContext;
}

TWeakObjectPtr<AFightingVRPlayerController> FightingVRHUDPCTrackerBase::GetPlayerController() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		APlayerController* PC = Context.GetPlayerController();
		AFightingVRPlayerController* FightingVRPC = Cast<AFightingVRPlayerController>(PC);
		return MakeWeakObjectPtr(FightingVRPC);
	}
	else
	{
		return NULL;
	}
}


UWorld* FightingVRHUDPCTrackerBase::GetWorld() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld();
	}
	else
	{
		return NULL;
	}
}

AFightingVRState* FightingVRHUDPCTrackerBase::GetGameState() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld()->GetGameState<AFightingVRState>();
	}
	else
	{
		return NULL;
	}
}

const FLocalPlayerContext& FightingVRHUDPCTrackerBase::GetContext() const
{
	return Context;
}



