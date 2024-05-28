// Copyright Epic Games, Inc. All Rights Reserved.

#include "Bots/FightingVRBot.h"
#include "FightingVR.h"
#include "Bots/FightingVRAIController.h"

AFightingVRBot::AFightingVRBot(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	AIControllerClass = AFightingVRAIController::StaticClass();

	UpdatePawnMeshes();

	bUseControllerRotationYaw = true;
}

bool AFightingVRBot::IsFirstPerson() const
{
	return false;
}

void AFightingVRBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}
