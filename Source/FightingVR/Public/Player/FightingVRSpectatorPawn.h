// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FightingVRSpectatorPawn.generated.h"


UCLASS(config = Game, Blueprintable, BlueprintType)
class AFightingVRSpectatorPawn : public ASpectatorPawn
{
	GENERATED_UCLASS_BODY()

	// Begin ASpectatorPawn overrides
	/** Overridden to implement Key Bindings the match the player controls */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End Pawn overrides
	
	// Frame rate linked look
	void LookUpAtRate(float Val);
};
