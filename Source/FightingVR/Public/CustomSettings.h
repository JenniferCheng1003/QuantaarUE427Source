// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CustomSettings.generated.h"


/**
 * 
 */

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Custom Game Settings"))
class FIGHTINGVR_API UCustomSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	// ** This project's class name * //
	static const FString ProjectName;

public:
	UPROPERTY(EditAnywhere, config, Category = Packaging)
	bool bWithCopyrightNotice = true;

	UPROPERTY(EditAnywhere, config, Category = Packaging)
	bool bWithUploadDatatoServer = true;

	UPROPERTY(EditAnywhere, config, Category = Packaging)
	bool bWithStandaloneHMD = true;

	UFUNCTION(BlueprintPure, Category = Packaging)
	static bool GetWithCopyrightNotice();

	UFUNCTION(BlueprintPure, Category = Packaging)
	static bool GetWithUploadDatatoServer();

	UFUNCTION(BlueprintPure, Category = Packaging)
	static bool GetWithStandaloneHMD();
};