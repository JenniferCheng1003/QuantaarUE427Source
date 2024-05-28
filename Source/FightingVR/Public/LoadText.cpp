// Fill out your copyright notice in the Description page of Project Settings.

#include "LoadText.h"

bool ULoadText::FileSaveString(FString SaveString, FString SavePath)
{
	return FFileHelper::SaveStringToFile(SaveString, *(FPaths::ProjectDir() + SavePath));
}

bool ULoadText::FileLoadString(FString LoadPath, FString& LoadString)
{
	return FFileHelper::LoadFileToString(LoadString, *(FPaths::ProjectDir() + LoadPath));
}

void ULoadText::ScoreRankSort(UPARAM(ref) TArray<FScoreData>& scoreData)
{
    scoreData.Sort([](const FScoreData& A, const FScoreData& B) {
		return A.scoreValue > B.scoreValue || (A.scoreValue == B.scoreValue && A.damageValue > B.damageValue) || (A.scoreValue == B.scoreValue && A.damageValue == B.damageValue && A.takenDamageValue < B.takenDamageValue);
	});
}
