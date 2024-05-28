// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoadText.generated.h"

USTRUCT(BlueprintType)
struct FScoreData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int masterNumber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float scoreValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float damageValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float takenDamageValue;
};

/**
 * 
 */
UCLASS()
class FIGHTINGVR_API ULoadText : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "save")
	static bool FileSaveString(FString SaveString, FString SavePath);

	UFUNCTION(BlueprintPure, Category = "save")
	static bool FileLoadString(FString LoadPath, FString& LoadString);

	UFUNCTION(BlueprintCallable, meta=(CompactNodeTitle = "ScoreRANK"))
	static void ScoreRankSort(UPARAM(ref) TArray<FScoreData>& scoreData);
};
