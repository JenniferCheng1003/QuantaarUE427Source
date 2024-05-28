// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomSettings.h"

// !!!!!!!!!!!!!
// !_IMPORTANT_!
// !!!!!!!!!!!!!
FString const UCustomSettings::ProjectName = "FightingVR";

bool UCustomSettings::GetWithCopyrightNotice()
{
	if (ProjectName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ProjectName must be set in CustomSettings.h"));
		return false;
	}
	else
	{
		const auto Path = FString::Printf(TEXT("/Script/%s.CustomSettings"), *ProjectName);

		bool bWithCopyrightNotice;
		GConfig->GetBool(
			*Path,
			TEXT("bWithCopyrightNotice"),
			bWithCopyrightNotice,
			GGameIni
		);

		return bWithCopyrightNotice;
	}
}

bool UCustomSettings::GetWithUploadDatatoServer()
{
	if (ProjectName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ProjectName must be set in CustomSettings.h"));

		return false;
	}
	else
	{
		const auto Path = FString::Printf(TEXT("/Script/%s.CustomSettings"), *ProjectName);

		bool bWithUploadDatatoServer;
		GConfig->GetBool(
			*Path,
			TEXT("bWithUploadDatatoServer"),
			bWithUploadDatatoServer,
			GGameIni
		);

		return bWithUploadDatatoServer;
	}
}

bool UCustomSettings::GetWithStandaloneHMD()
{
	if (ProjectName.IsEmpty())
		UE_LOG(LogTemp, Error, TEXT("ProjectName must be set in CustomSettings.h"));

	const auto Path = FString::Printf(TEXT("/Script/%s.CustomSettings"), *ProjectName);

	bool bWithStandaloneHMD;
	GConfig->GetBool(
		*Path,
		TEXT("bWithStandaloneHMD"),
		bWithStandaloneHMD,
		GGameIni
	);

	return bWithStandaloneHMD;
}