// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRDemoSpectator.h"
#include "FightingVR.h"
#include "UI/Menu/FightingVRDemoPlaybackMenu.h"
#include "UI/Widgets/SFightingVRDemoHUD.h"
#include "Engine/DemoNetDriver.h"

AFightingVRDemoSpectator::AFightingVRDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowMouseCursor = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void AFightingVRDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "InGameMenu", IE_Pressed, this, &AFightingVRDemoSpectator::OnToggleInGameMenu );

	InputComponent->BindAction( "NextWeapon", IE_Pressed, this, &AFightingVRDemoSpectator::OnIncreasePlaybackSpeed );
	InputComponent->BindAction( "PrevWeapon", IE_Pressed, this, &AFightingVRDemoSpectator::OnDecreasePlaybackSpeed );
}

void AFightingVRDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	// Build menu only after game is initialized
	FightingVRDemoPlaybackMenu = MakeShareable( new FFightingVRDemoPlaybackMenu() );
	FightingVRDemoPlaybackMenu->Construct( Cast< ULocalPlayer >( Player ) );

	// Create HUD if this is playback
	if (GetWorld() != nullptr && GetWorld()->GetDemoNetDriver() != nullptr && !GetWorld()->GetDemoNetDriver()->IsServer())
	{
		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			DemoHUD = SNew(SFightingVRDemoHUD)
				.PlayerOwner(this);

			GEngine->GameViewport->AddViewportWidgetContent(DemoHUD.ToSharedRef());
		}
	}

	FActorSpawnParameters SpawnInfo;

	SpawnInfo.Owner				= this;
	SpawnInfo.Instigator		= GetInstigator();
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PlaybackSpeed = 2;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(DemoHUD);

	SetInputMode(InputMode);
}

void AFightingVRDemoSpectator::OnToggleInGameMenu()
{
	// if no one's paused, pause
	if ( FightingVRDemoPlaybackMenu.IsValid() )
	{
		FightingVRDemoPlaybackMenu->ToggleGameMenu();
	}
}

static float PlaybackSpeedLUT[5] = { 0.1f, 0.5f, 1.0f, 2.0f, 4.0f };

void AFightingVRDemoSpectator::OnIncreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed + 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AFightingVRDemoSpectator::OnDecreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed - 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AFightingVRDemoSpectator::Destroyed()
{
	if (GEngine != nullptr && GEngine->GameViewport != nullptr && DemoHUD.IsValid())
	{
		// Remove HUD
		GEngine->GameViewport->RemoveViewportWidgetContent(DemoHUD.ToSharedRef());
	}

	Super::Destroyed();
}
