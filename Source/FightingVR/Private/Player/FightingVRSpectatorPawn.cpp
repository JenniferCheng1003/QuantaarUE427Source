// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/FightingVRSpectatorPawn.h"
#include "FightingVR.h"

AFightingVRSpectatorPawn::AFightingVRSpectatorPawn(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = false;
}

void AFightingVRSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
	PlayerInputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFightingVRSpectatorPawn::LookUpAtRate);
}

void AFightingVRSpectatorPawn::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}
