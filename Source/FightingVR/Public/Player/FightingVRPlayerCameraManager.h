// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FightingVRPlayerCameraManager.generated.h"

UCLASS()
class AFightingVRPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_UCLASS_BODY()

public:

	/** normal FOV */
	float NormalFOV;

	/** targeting FOV */
	float TargetingFOV;

	/** After updating camera, inform pawn to update 1p mesh to match camera's location&rotation */
	virtual void UpdateCamera(float DeltaTime) override;
};