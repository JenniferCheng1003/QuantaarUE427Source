// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRCharacter.h"
#include "FightingVRBot.generated.h"

UCLASS()
class AFightingVRBot : public AFightingVRCharacter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* BotBehavior;

	virtual bool IsFirstPerson() const override;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;
};