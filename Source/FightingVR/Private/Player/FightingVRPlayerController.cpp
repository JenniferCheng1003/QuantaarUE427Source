// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/FightingVRPlayerController.h"
#include "FightingVR.h"
#include "Player/FightingVRPlayerCameraManager.h"
#include "Player/FightingVRCheatManager.h"
#include "Player/FightingVRLocalPlayer.h"
#include "Online/FightingVRPlayerState.h"
#include "Weapons/FightingVRWeapon.h"
#include "UI/Menu/FightingVRIngameMenu.h"
#include "UI/Style/FightingVRStyle.h"
#include "UI/FightingVRHUD.h"
#include "Online.h"
#include "OnlineAchievementsInterface.h"
#include "OnlineEventsInterface.h"
#include "OnlineStatsInterface.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSessionInterface.h"
#include "FightingVRInstance.h"
#include "FightingVRLeaderboards.h"
#include "FightingVRViewportClient.h"
#include "Sound/SoundNodeLocalPlayer.h"
#include "AudioThread.h"
#include "OnlineSubsystemUtils.h"

#define  ACH_FRAG_SOMEONE	TEXT("ACH_FRAG_SOMEONE")
#define  ACH_SOME_KILLS		TEXT("ACH_SOME_KILLS")
#define  ACH_LOTS_KILLS		TEXT("ACH_LOTS_KILLS")
#define  ACH_FINISH_MATCH	TEXT("ACH_FINISH_MATCH")
#define  ACH_LOTS_MATCHES	TEXT("ACH_LOTS_MATCHES")
#define  ACH_FIRST_WIN		TEXT("ACH_FIRST_WIN")
#define  ACH_LOTS_WIN		TEXT("ACH_LOTS_WIN")
#define  ACH_MANY_WIN		TEXT("ACH_MANY_WIN")
#define  ACH_SHOOT_BULLETS	TEXT("ACH_SHOOT_BULLETS")
#define  ACH_SHOOT_ROCKETS	TEXT("ACH_SHOOT_ROCKETS")
#define  ACH_GOOD_SCORE		TEXT("ACH_GOOD_SCORE")
#define  ACH_GREAT_SCORE	TEXT("ACH_GREAT_SCORE")
#define  ACH_PLAY_SANCTUARY	TEXT("ACH_PLAY_SANCTUARY")
#define  ACH_PLAY_HIGHRISE	TEXT("ACH_PLAY_HIGHRISE")

static const int32 SomeKillsCount = 10;
static const int32 LotsKillsCount = 20;
static const int32 LotsMatchesCount = 5;
static const int32 LotsWinsCount = 3;
static const int32 ManyWinsCount = 5;
static const int32 LotsBulletsCount = 100;
static const int32 LotsRocketsCount = 10;
static const int32 GoodScoreCount = 10;
static const int32 GreatScoreCount = 15;

#if !defined(TRACK_STATS_LOCALLY)
#define TRACK_STATS_LOCALLY 1
#endif

AFightingVRPlayerController::AFightingVRPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerCameraManagerClass = AFightingVRPlayerCameraManager::StaticClass();
	CheatClass = UFightingVRCheatManager::StaticClass();
	bAllowGameActions = true;
	bGameEndedFrame = false;
	LastDeathLocation = FVector::ZeroVector;

	ServerSayString = TEXT("Say");
	FightingVRFriendUpdateTimer = 0.0f;
	bHasSentStartEvents = false;

	StatMatchesPlayed = 0;
	StatKills = 0;
	StatDeaths = 0;
	bHasQueriedPlatformStats = false;
	bHasQueriedPlatformAchievements = false;
	bHasInitializedInputComponent = false;
}

void AFightingVRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(!bHasInitializedInputComponent)
	{
		// UI input
		InputComponent->BindAction("InGameMenu", IE_Pressed, this, &AFightingVRPlayerController::OnToggleInGameMenu);
		InputComponent->BindAction("Scoreboard", IE_Pressed, this, &AFightingVRPlayerController::OnShowScoreboard);
		InputComponent->BindAction("Scoreboard", IE_Released, this, &AFightingVRPlayerController::OnHideScoreboard);
		InputComponent->BindAction("ConditionalCloseScoreboard", IE_Pressed, this, &AFightingVRPlayerController::OnConditionalCloseScoreboard);
		InputComponent->BindAction("ToggleScoreboard", IE_Pressed, this, &AFightingVRPlayerController::OnToggleScoreboard);

		// voice chat
		InputComponent->BindAction("PushToTalk", IE_Pressed, this, &APlayerController::StartTalking);
		InputComponent->BindAction("PushToTalk", IE_Released, this, &APlayerController::StopTalking);

		InputComponent->BindAction("ToggleChat", IE_Pressed, this, &AFightingVRPlayerController::ToggleChatWindow);

		bHasInitializedInputComponent = true;
	}
}


void AFightingVRPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	FFightingVRStyle::Initialize();
	FightingVRFriendUpdateTimer = 0;
}

void AFightingVRPlayerController::ClearLeaderboardDelegate()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
		if (Leaderboards.IsValid())
		{
			Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
		}
	}
}

void AFightingVRPlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (IsGameMenuVisible())
	{
		if (FightingVRFriendUpdateTimer > 0)
		{
			FightingVRFriendUpdateTimer -= DeltaTime;
		}
		else
		{
			TSharedPtr<class FFightingVRFriends> FightingVRFriends = FightingVRIngameMenu->GetFightingVRFriends();
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
			if (FightingVRFriends.IsValid() && LocalPlayer && LocalPlayer->GetControllerId() >= 0)
			{
				FightingVRFriends->UpdateFriends(LocalPlayer->GetControllerId());
			}
			
			// Make sure the time between calls is long enough that we won't trigger (0x80552C81) and not exceed the web api rate limit
			// That value is currently 75 requests / 15 minutes.
			FightingVRFriendUpdateTimer = 15;

		}
	}

	// Is this the first frame after the game has ended
	if(bGameEndedFrame)
	{
		bGameEndedFrame = false;

		// ONLY PUT CODE HERE WHICH YOU DON'T WANT TO BE DONE DUE TO HOST LOSS

		// Do we need to show the end of round scoreboard?
		if (IsPrimaryPlayer())
		{
			AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
			if (FightingVRHUD)
			{
				FightingVRHUD->ShowScoreboard(true, true);
			}
		}
	}

	const bool bLocallyControlled = IsLocalController();
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
		USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
};

void AFightingVRPlayerController::BeginDestroy()
{
	Super::BeginDestroy();
	ClearLeaderboardDelegate();

	// clear any online subsystem references
	FightingVRIngameMenu = nullptr;

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void AFightingVRPlayerController::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	if (ULocalPlayer* const LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		//Build menu only after game is initialized
		FightingVRIngameMenu = MakeShareable(new FFightingVRIngameMenu());
		FightingVRIngameMenu->Construct(Cast<ULocalPlayer>(Player));

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void AFightingVRPlayerController::QueryAchievements()
{
	if (bHasQueriedPlatformAchievements)
	{
		return;
	}
	bHasQueriedPlatformAchievements = true;
	// precache achievements
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();

					if (Achievements.IsValid())
					{
						Achievements->QueryAchievements( *UserId.Get(), FOnQueryAchievementsCompleteDelegate::CreateUObject( this, &AFightingVRPlayerController::OnQueryAchievementsComplete ));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot read achievements."));
	}
}

void AFightingVRPlayerController::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful )
{
	UE_LOG(LogOnline, Display, TEXT("AFightingVRPlayerController::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void AFightingVRPlayerController::OnLeaderboardReadComplete(bool bWasSuccessful)
{
	if (ReadObject.IsValid() && ReadObject->ReadState == EOnlineAsyncTaskState::Done && !bHasQueriedPlatformStats)
	{
		bHasQueriedPlatformStats = true;
		ClearLeaderboardDelegate();

		// We should only have one stat.
		if (bWasSuccessful && ReadObject->Rows.Num() == 1)
		{
			FOnlineStatsRow& RowData = ReadObject->Rows[0];
			if (const FVariantData* KillData = RowData.Columns.Find(LEADERBOARD_STAT_KILLS))
			{
				KillData->GetValue(StatKills);
			}

			if (const FVariantData* DeathData = RowData.Columns.Find(LEADERBOARD_STAT_DEATHS))
			{
				DeathData->GetValue(StatDeaths);
			}

			if (const FVariantData* MatchData = RowData.Columns.Find(LEADERBOARD_STAT_MATCHESPLAYED))
			{
				MatchData->GetValue(StatMatchesPlayed);
			}

			UE_LOG(LogOnline, Log, TEXT("Fetched player stat data. Kills %d Deaths %d Matches %d"), StatKills, StatDeaths, StatMatchesPlayed);
		}
	}
}

void AFightingVRPlayerController::QueryStats()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid() && !bHasQueriedPlatformStats)
					{
						TArray<TSharedRef<const FUniqueNetId>> QueryPlayers;
						QueryPlayers.Add(UserId.ToSharedRef());

						LeaderboardReadCompleteDelegateHandle = Leaderboards->OnLeaderboardReadCompleteDelegates.AddUObject(this, &AFightingVRPlayerController::OnLeaderboardReadComplete);
						ReadObject = MakeShareable(new FFightingVRAllTimeMatchResultsRead());
						FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();
						if (Leaderboards->ReadLeaderboards(QueryPlayers, ReadObjectRef))
						{
							UE_LOG(LogOnline, Log, TEXT("Started process to fetch stats for current user."));
						}
						else
						{
							UE_LOG(LogOnline, Warning, TEXT("Could not start leaderboard fetch process. This will affect stat writes for this session."));
						}
						
					}
				}
			}
		}
	}
}

void AFightingVRPlayerController::UnFreeze()
{
	ServerRestartPlayer();
}

void AFightingVRPlayerController::FailedToSpawnPawn()
{
	if(StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

void AFightingVRPlayerController::PawnPendingDestroy(APawn* P)
{
	LastDeathLocation = P->GetActorLocation();
	FVector CameraLocation = LastDeathLocation + FVector(0, 0, 300.0f);
	FRotator CameraRotation(-90.0f, 0.0f, 0.0f);
	FindDeathCameraSpot(CameraLocation, CameraRotation);

	Super::PawnPendingDestroy(P);

	ClientSetSpectatorCamera(CameraLocation, CameraRotation);
}

void AFightingVRPlayerController::GameHasEnded(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void AFightingVRPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

bool AFightingVRPlayerController::FindDeathCameraSpot(FVector& CameraLocation, FRotator& CameraRotation)
{
	const FVector PawnLocation = GetPawn()->GetActorLocation();
	FRotator ViewDir = GetControlRotation();
	ViewDir.Pitch = -45.0f;

	const float YawOffsets[] = { 0.0f, -180.0f, 90.0f, -90.0f, 45.0f, -45.0f, 135.0f, -135.0f };
	const float CameraOffset = 600.0f;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DeathCamera), true, GetPawn());

	FHitResult HitResult;
	for (int32 i = 0; i < UE_ARRAY_COUNT(YawOffsets); i++)
	{
		FRotator CameraDir = ViewDir;
		CameraDir.Yaw += YawOffsets[i];
		CameraDir.Normalize();

		const FVector TestLocation = PawnLocation - CameraDir.Vector() * CameraOffset;
		
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, PawnLocation, TestLocation, ECC_Camera, TraceParams);

		if (!bBlocked)
		{
			CameraLocation = TestLocation;
			CameraRotation = CameraDir;
			return true;
		}
	}

	return false;
}

bool AFightingVRPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}

void AFightingVRPlayerController::ServerCheat_Implementation(const FString& Msg)
{
	if (CheatManager)
	{
		ClientMessage(ConsoleCommand(Msg));
	}
}

void AFightingVRPlayerController::SimulateInputKey(FKey Key, bool bPressed)
{
	InputKey(Key, bPressed ? IE_Pressed : IE_Released, 1, false);
}

void AFightingVRPlayerController::OnKill()
{
	UpdateAchievementProgress(ACH_FRAG_SOMEONE, 100.0f);

	const UWorld* World = GetWorld();

	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

	if (Events.IsValid() && Identity.IsValid())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		if (LocalPlayer)
		{
			int32 UserIndex = LocalPlayer->GetControllerId();
			TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);			
			if (UniqueID.IsValid())
			{			
				AFightingVRCharacter* FightingVRChar = Cast<AFightingVRCharacter>(GetCharacter());
				// If player is dead, use location stored during pawn cleanup.
				FVector Location = FightingVRChar ? FightingVRChar->GetActorLocation() : LastDeathLocation;
				AFightingVRWeapon* Weapon = FightingVRChar ? FightingVRChar->GetWeapon() : 0;
				int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;

				FOnlineEventParms Params;		

				Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

				Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
				Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked			
				Params.Add( TEXT( "KillTypeId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
				Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
				Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
			
				Events->TriggerEvent(*UniqueID, TEXT("KillOponent"), Params);				
			}
		}
	}
}

void AFightingVRPlayerController::OnDeathMessage(class AFightingVRPlayerState* KillerPlayerState, class AFightingVRPlayerState* KilledPlayerState, const UDamageType* KillerDamageType) 
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if (FightingVRHUD)
	{
		FightingVRHUD->ShowDeathMessage(KillerPlayerState, KilledPlayerState, KillerDamageType);		
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetCachedUniqueNetId().IsValid() && KilledPlayerState->GetUniqueId().IsValid())
	{
		// if this controller is the player who died, update the hero stat.
		if (*LocalPlayer->GetCachedUniqueNetId() == *KilledPlayerState->GetUniqueId())
		{
			const UWorld* World = GetWorld();
			const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
			const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

			if (Events.IsValid() && Identity.IsValid())
			{							
				const int32 UserIndex = LocalPlayer->GetControllerId();
				TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
				if (UniqueID.IsValid())
				{				
					AFightingVRCharacter* FightingVRChar = Cast<AFightingVRCharacter>(GetCharacter());
					AFightingVRWeapon* Weapon = FightingVRChar ? FightingVRChar->GetWeapon() : NULL;

					FVector Location = FightingVRChar ? FightingVRChar->GetActorLocation() : FVector::ZeroVector;
					int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;

					FOnlineEventParms Params;
					Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
					Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

					Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
					Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked
				
					Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
					Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
					Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
										
					Events->TriggerEvent(*UniqueID, TEXT("PlayerDeath"), Params);
				}
			}
		}
	}	
}

void AFightingVRPlayerController::UpdateAchievementProgress( const FString& Id, float Percent )
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				FUniqueNetIdRepl UserId = LocalPlayer->GetCachedUniqueNetId();

				if (UserId.IsValid())
				{

					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
					if (Achievements.IsValid() && (!WriteObject.IsValid() || WriteObject->WriteState != EOnlineAsyncTaskState::InProgress))
					{
						WriteObject = MakeShareable(new FOnlineAchievementsWrite());
						WriteObject->SetFloatStat(*Id, Percent);

						FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
						Achievements->WriteAchievements(*UserId, WriteObjectRef);
					}
					else
					{
						UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot update achievements."));
	}
}

void AFightingVRPlayerController::OnToggleInGameMenu()
{
	if( GEngine->GameViewport == nullptr )
	{
		return;
	}

	// this is not ideal, but necessary to prevent both players from pausing at the same time on the same frame
	UWorld* GameWorld = GEngine->GameViewport->GetWorld();

	for(auto It = GameWorld->GetControllerIterator(); It; ++It)
	{
		AFightingVRPlayerController* Controller = Cast<AFightingVRPlayerController>(*It);
		if(Controller && Controller->IsPaused())
		{
			return;
		}
	}

	// if no one's paused, pause
	if (FightingVRIngameMenu.IsValid())
	{
		FightingVRIngameMenu->ToggleGameMenu();
	}
}

void AFightingVRPlayerController::OnConditionalCloseScoreboard()
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if(FightingVRHUD && ( FightingVRHUD->IsMatchOver() == false ))
	{
		FightingVRHUD->ConditionalCloseScoreboard();
	}
}

void AFightingVRPlayerController::OnToggleScoreboard()
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if(FightingVRHUD && ( FightingVRHUD->IsMatchOver() == false ))
	{
		FightingVRHUD->ToggleScoreboard();
	}
}

void AFightingVRPlayerController::OnShowScoreboard()
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if(FightingVRHUD)
	{
		FightingVRHUD->ShowScoreboard(true);
	}
}

void AFightingVRPlayerController::OnHideScoreboard()
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	// If have a valid match and the match is over - hide the scoreboard
	if( (FightingVRHUD != NULL ) && ( FightingVRHUD->IsMatchOver() == false ) )
	{
		FightingVRHUD->ShowScoreboard(false);
	}
}

bool AFightingVRPlayerController::IsGameMenuVisible() const
{
	bool Result = false; 
	if (FightingVRIngameMenu.IsValid())
	{
		Result = FightingVRIngameMenu->GetIsGameMenuUp();
	} 

	return Result;
}

void AFightingVRPlayerController::SetInfiniteAmmo(bool bEnable)
{
	bInfiniteAmmo = bEnable;
}

void AFightingVRPlayerController::SetInfiniteClip(bool bEnable)
{
	bInfiniteClip = bEnable;
}

void AFightingVRPlayerController::SetHealthRegen(bool bEnable)
{
	bHealthRegen = bEnable;
}

void AFightingVRPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

void AFightingVRPlayerController::SetIsVibrationEnabled(bool bEnable)
{
	bIsVibrationEnabled = bEnable;
}

void AFightingVRPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);

	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if (FightingVRHUD)
	{
		FightingVRHUD->SetMatchState(EFightingVRMatchState::Playing);
		FightingVRHUD->ShowScoreboard(false);
	}
	bGameEndedFrame = false;


	QueryAchievements();

	QueryStats();


	const UWorld* World = GetWorld();

	// Send round start event
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);

			// Online matches require the MultiplayerRoundStart event as well
			UFightingVRInstance* SGI = Cast<UFightingVRInstance>(World->GetGameInstance());

			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;

				// @todo: fill in with real values
				MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
				
				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundStart"), MultiplayerParams);
			}

			bHasSentStartEvents = true;
		}
	}
}

/** Starts the online game using the session name in the PlayerState */
void AFightingVRPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
	if (FightingVRPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(FightingVRPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *FightingVRPlayerState->SessionName.ToString() );
				Sessions->StartSession(FightingVRPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClientStartOnlineGame, this, &AFightingVRPlayerController::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

/** Ends the online game using the session name in the PlayerState */
void AFightingVRPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
	if (FightingVRPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(FightingVRPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *FightingVRPlayerState->SessionName.ToString() );
				Sessions->EndSession(FightingVRPlayerState->SessionName);
			}
		}
	}
}

void AFightingVRPlayerController::HandleReturnToMainMenu()
{
	OnHideScoreboard();
	CleanupSessionOnReturnToMenu();
}

void AFightingVRPlayerController::ClientReturnToMainMenu_Implementation(const FString& InReturnReason)
{		
	const UWorld* World = GetWorld();
	UFightingVRInstance* SGI = World != NULL ? Cast<UFightingVRInstance>(World->GetGameInstance()) : NULL;

	if ( !ensure( SGI != NULL ) )
	{
		return;
	}

	if ( GetNetMode() == NM_Client )
	{
		const FText ReturnReason	= NSLOCTEXT( "NetworkErrors", "HostQuit", "The host has quit the match." );
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		SGI->ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), FightingVRInstanceState::MainMenu );
	}
	else
	{
		SGI->GotoState(FightingVRInstanceState::MainMenu);
	}

	// Clear the flag so we don't do normal end of round stuff next
	bGameEndedFrame = false;
}

/** Ends and/or destroys game session */
void AFightingVRPlayerController::CleanupSessionOnReturnToMenu()
{
	const UWorld* World = GetWorld();
	UFightingVRInstance * SGI = World != NULL ? Cast<UFightingVRInstance>( World->GetGameInstance() ) : NULL;

	if ( ensure( SGI != NULL ) )
	{
		SGI->CleanupSessionOnReturnToMenu();
	}
}

void AFightingVRPlayerController::ClientGameEnded_Implementation(class AActor* EndGameFocus, bool bIsWinner)
{
	Super::ClientGameEnded_Implementation(EndGameFocus, bIsWinner);
	
	// Disable controls now the game has ended
	SetIgnoreMoveInput(true);

	bAllowGameActions = false;

	// Make sure that we still have valid view target
	SetViewTarget(GetPawn());

	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();
	if (FightingVRHUD)
	{
		FightingVRHUD->SetMatchState(bIsWinner ? EFightingVRMatchState::Won : EFightingVRMatchState::Lost);
	}

	UpdateSaveFileOnGameEnd(bIsWinner);
	UpdateAchievementsOnGameEnd();
	UpdateLeaderboardsOnGameEnd();
	UpdateStatsOnGameEnd(bIsWinner);

	// Flag that the game has just ended (if it's ended due to host loss we want to wait for ClientReturnToMainMenu_Implementation first, incase we don't want to process)
	bGameEndedFrame = true;
}

void AFightingVRPlayerController::ClientSendRoundEndEvent_Implementation(bool bIsWinner, int32 ExpendedTimeInSeconds)
{
	const UWorld* World = GetWorld();
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(bHasSentStartEvents && LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{	
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
			AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
			int32 PlayerScore = FightingVRPlayerState ? FightingVRPlayerState->GetScore() : 0;
			int32 PlayerDeaths = FightingVRPlayerState ? FightingVRPlayerState->GetDeaths() : 0;
			int32 PlayerKills = FightingVRPlayerState ? FightingVRPlayerState->GetKills() : 0;
			
			// Fire session end event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "PlayerScore" ), FVariantData( (int32)PlayerScore ) );
			Params.Add( TEXT( "PlayerWon" ), FVariantData( (bool)bIsWinner ) );
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			Params.Add( TEXT( "MapNameString" ), FVariantData( MapName ) ); // @todo workaround for a bug in backend service, remove when fixed
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionEnd"), Params);

			// Update all time results
			FOnlineEventParms AllTimeMatchParams;
			AllTimeMatchParams.Add(TEXT("FightingVRAllTimeMatchResultsScore"), FVariantData((uint64)PlayerScore));
			AllTimeMatchParams.Add(TEXT("FightingVRAllTimeMatchResultsDeaths"), FVariantData((int32)PlayerDeaths));
			AllTimeMatchParams.Add(TEXT("FightingVRAllTimeMatchResultsFrags"), FVariantData((int32)PlayerKills));
			AllTimeMatchParams.Add(TEXT("FightingVRAllTimeMatchResultsMatchesPlayed"), FVariantData((int32)1));

			Events->TriggerEvent(*UniqueId, TEXT("FightingVRAllTimeMatchResults"), AllTimeMatchParams);

			// Online matches require the MultiplayerRoundEnd event as well
			UFightingVRInstance* SGI = Cast<UFightingVRInstance>(World->GetGameInstance());
			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;
				MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "TimeInSeconds" ), FVariantData( (float)ExpendedTimeInSeconds ) );
				MultiplayerParams.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
					
				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundEnd"), MultiplayerParams);
			}
		}

		bHasSentStartEvents = false;
	}
}

void AFightingVRPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);

	// If we have a pawn we need to determine if we should show/hide the weapon
	AFightingVRCharacter* MyPawn = Cast<AFightingVRCharacter>(GetPawn());
	AFightingVRWeapon* MyWeapon = MyPawn ? MyPawn->GetWeapon() : NULL;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}

bool AFightingVRPlayerController::IsMoveInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsMoveInputIgnored();
	}
}

bool AFightingVRPlayerController::IsLookInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsLookInputIgnored();
	}
}

void AFightingVRPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	UFightingVRPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

void AFightingVRPlayerController::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( AFightingVRPlayerController, bInfiniteAmmo, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AFightingVRPlayerController, bInfiniteClip, COND_OwnerOnly );

	DOREPLIFETIME(AFightingVRPlayerController, bHealthRegen);
}

void AFightingVRPlayerController::Suicide()
{
	if ( IsInState(NAME_Playing) )
	{
		ServerSuicide();
	}
}

bool AFightingVRPlayerController::ServerSuicide_Validate()
{
	return true;
}

void AFightingVRPlayerController::ServerSuicide_Implementation()
{
	if ( (GetPawn() != NULL) && ((GetWorld()->TimeSeconds - GetPawn()->CreationTime > 1) || (GetNetMode() == NM_Standalone)) )
	{
		AFightingVRCharacter* MyPawn = Cast<AFightingVRCharacter>(GetPawn());
		if (MyPawn)
		{
			MyPawn->Suicide();
		}
	}
}

bool AFightingVRPlayerController::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

bool AFightingVRPlayerController::HasInfiniteClip() const
{
	return bInfiniteClip;
}

bool AFightingVRPlayerController::HasHealthRegen() const
{
	return bHealthRegen;
}

bool AFightingVRPlayerController::HasGodMode() const
{
	return bGodMode;
}

bool AFightingVRPlayerController::IsVibrationEnabled() const
{
	return bIsVibrationEnabled;
}

bool AFightingVRPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions && !bCinematicMode;
}

void AFightingVRPlayerController::ToggleChatWindow()
{
	AFightingVRHUD* FightingVRHUD = Cast<AFightingVRHUD>(GetHUD());
	if (FightingVRHUD)
	{
		FightingVRHUD->ToggleChat();
	}
}

void AFightingVRPlayerController::ClientTeamMessage_Implementation( APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime  )
{
	AFightingVRHUD* FightingVRHUD = Cast<AFightingVRHUD>(GetHUD());
	if (FightingVRHUD)
	{
		if( Type == ServerSayString )
		{
			if( SenderPlayerState != PlayerState  )
			{
				FightingVRHUD->AddChatLine(FText::FromString(S), false);
			}
		}
	}
}

void AFightingVRPlayerController::Say( const FString& Msg )
{
	ServerSay(Msg.Left(128));
}

bool AFightingVRPlayerController::ServerSay_Validate( const FString& Msg )
{
	return true;
}

void AFightingVRPlayerController::ServerSay_Implementation( const FString& Msg )
{
	GetWorld()->GetAuthGameMode<AFightingVRMode>()->Broadcast(this, Msg, ServerSayString);
}

AFightingVRHUD* AFightingVRPlayerController::GetFightingVRHUD() const
{
	return Cast<AFightingVRHUD>(GetHUD());
}


UFightingVRPersistentUser* AFightingVRPlayerController::GetPersistentUser() const
{
	UFightingVRLocalPlayer* const FightingVRLocalPlayer = Cast<UFightingVRLocalPlayer>(Player);
	return FightingVRLocalPlayer ? FightingVRLocalPlayer->GetPersistentUser() : nullptr;
}

bool AFightingVRPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate /*= FCanUnpause()*/)
{
	const bool Result = APlayerController::SetPause(bPause, CanUnpauseDelegate);

	// Update rich presence.
	const UWorld* World = GetWorld();
	const IOnlinePresencePtr PresenceInterface = Online::GetPresenceInterface(World);
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	FUniqueNetIdRepl UserId = LocalPlayer ? LocalPlayer->GetCachedUniqueNetId() : FUniqueNetIdRepl();

	// Don't send pause events while online since the game doesn't actually pause
	if(GetNetMode() == NM_Standalone && Events.IsValid() && PlayerState->GetUniqueId().IsValid())
	{
		FOnlineEventParms Params;
		Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
		Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
		if(Result && bPause)
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionPause"), Params);
		}
		else
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionResume"), Params);
		}
	}

	return Result;
}

FVector AFightingVRPlayerController::GetFocalLocation() const
{
	const AFightingVRCharacter* FightingVRCharacter = Cast<AFightingVRCharacter>(GetPawn());

	// On death we want to use the player's death cam location rather than the location of where the pawn is at the moment
	// This guarantees that the clients see their death cam correctly, as their pawns have delayed destruction.
	if (FightingVRCharacter && FightingVRCharacter->bIsDying)
	{
		return GetSpawnLocation();
	}

	return Super::GetFocalLocation();
}

void AFightingVRPlayerController::ShowInGameMenu()
{
	AFightingVRHUD* FightingVRHUD = GetFightingVRHUD();	
	if(FightingVRIngameMenu.IsValid() && !FightingVRIngameMenu->GetIsGameMenuUp() && FightingVRHUD && (FightingVRHUD->IsMatchOver() == false))
	{
		FightingVRIngameMenu->ToggleGameMenu();
	}
}
void AFightingVRPlayerController::UpdateAchievementsOnGameEnd()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
		if (FightingVRPlayerState)
		{			
			const UFightingVRPersistentUser*  PersistentUser = GetPersistentUser();

			if (PersistentUser)
			{						
				const int32 Wins = PersistentUser->GetWins();
				const int32 Losses = PersistentUser->GetLosses();
				const int32 Matches = Wins + Losses;

				const int32 TotalKills = PersistentUser->GetKills();
				const int32 MatchScore = (int32)FightingVRPlayerState->GetScore();

				const int32 TotalBulletsFired = PersistentUser->GetBulletsFired();
				const int32 TotalRocketsFired = PersistentUser->GetRocketsFired();
			
				float TotalGameAchievement = 0;
				float CurrentGameAchievement = 0;
			
				///////////////////////////////////////
				// Kill achievements
				if (TotalKills >= 1)
				{
					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{
					float fSomeKillPct = ((float)TotalKills / (float)SomeKillsCount) * 100.0f;
					fSomeKillPct = FMath::RoundToFloat(fSomeKillPct);
					UpdateAchievementProgress(ACH_SOME_KILLS, fSomeKillPct);

					CurrentGameAchievement += FMath::Min(fSomeKillPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsKillPct = ((float)TotalKills / (float)LotsKillsCount) * 100.0f;
					fLotsKillPct = FMath::RoundToFloat(fLotsKillPct);
					UpdateAchievementProgress(ACH_LOTS_KILLS, fLotsKillPct);

					CurrentGameAchievement += FMath::Min(fLotsKillPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Match Achievements
				{
					UpdateAchievementProgress(ACH_FINISH_MATCH, 100.0f);

					CurrentGameAchievement += 100;
					TotalGameAchievement += 100;
				}
			
				{
					float fLotsRoundsPct = ((float)Matches / (float)LotsMatchesCount) * 100.0f;
					fLotsRoundsPct = FMath::RoundToFloat(fLotsRoundsPct);
					UpdateAchievementProgress(ACH_LOTS_MATCHES, fLotsRoundsPct);

					CurrentGameAchievement += FMath::Min(fLotsRoundsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Win Achievements
				if (Wins >= 1)
				{
					UpdateAchievementProgress(ACH_FIRST_WIN, 100.0f);

					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{			
					float fLotsWinPct = ((float)Wins / (float)LotsWinsCount) * 100.0f;
					fLotsWinPct = FMath::RoundToInt(fLotsWinPct);
					UpdateAchievementProgress(ACH_LOTS_WIN, fLotsWinPct);

					CurrentGameAchievement += FMath::Min(fLotsWinPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{			
					float fManyWinPct = ((float)Wins / (float)ManyWinsCount) * 100.0f;
					fManyWinPct = FMath::RoundToInt(fManyWinPct);
					UpdateAchievementProgress(ACH_MANY_WIN, fManyWinPct);

					CurrentGameAchievement += FMath::Min(fManyWinPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Ammo Achievements
				{
					float fLotsBulletsPct = ((float)TotalBulletsFired / (float)LotsBulletsCount) * 100.0f;
					fLotsBulletsPct = FMath::RoundToFloat(fLotsBulletsPct);
					UpdateAchievementProgress(ACH_SHOOT_BULLETS, fLotsBulletsPct);

					CurrentGameAchievement += FMath::Min(fLotsBulletsPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsRocketsPct = ((float)TotalRocketsFired / (float)LotsRocketsCount) * 100.0f;
					fLotsRocketsPct = FMath::RoundToFloat(fLotsRocketsPct);
					UpdateAchievementProgress(ACH_SHOOT_ROCKETS, fLotsRocketsPct);

					CurrentGameAchievement += FMath::Min(fLotsRocketsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Score Achievements
				{
					float fGoodScorePct = ((float)MatchScore / (float)GoodScoreCount) * 100.0f;
					fGoodScorePct = FMath::RoundToFloat(fGoodScorePct);
					UpdateAchievementProgress(ACH_GOOD_SCORE, fGoodScorePct);
				}

				{
					float fGreatScorePct = ((float)MatchScore / (float)GreatScoreCount) * 100.0f;
					fGreatScorePct = FMath::RoundToFloat(fGreatScorePct);
					UpdateAchievementProgress(ACH_GREAT_SCORE, fGreatScorePct);
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Map Play Achievements
				UWorld* World = GetWorld();
				if (World)
				{			
					FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
					if (MapName.Find(TEXT("Highrise")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_HIGHRISE, 100.0f);
					}
					else if (MapName.Find(TEXT("Sanctuary")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_SANCTUARY, 100.0f);
					}
				}
				///////////////////////////////////////			

				const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
				const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

				if (Events.IsValid() && Identity.IsValid())
				{							
					const int32 UserIndex = LocalPlayer->GetControllerId();
					TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
					if (UniqueID.IsValid())
					{				
						FOnlineEventParms Params;

						float fGamePct = (CurrentGameAchievement / TotalGameAchievement) * 100.0f;
						fGamePct = FMath::RoundToFloat(fGamePct);
						Params.Add( TEXT( "CompletionPercent" ), FVariantData( (float)fGamePct ) );
						if (UniqueID.IsValid())
						{				
							Events->TriggerEvent(*UniqueID, TEXT("GameProgress"), Params);
						}
					}
				}
			}
		}
	}
}

void AFightingVRPlayerController::UpdateLeaderboardsOnGameEnd()
{
	UFightingVRLocalPlayer* LocalPlayer = Cast<UFightingVRLocalPlayer>(Player);
	if (LocalPlayer)
	{
		// update leaderboards - note this does not respect existing scores and overwrites them. We would first need to read the leaderboards if we wanted to do that.
		IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());
				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid())
					{
						AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
						if (FightingVRPlayerState)
						{
							FFightingVRAllTimeMatchResultsWrite ResultsWriteObject;
							int32 MatchWriteData = 1;
							int32 KillsWriteData = FightingVRPlayerState->GetKills();
							int32 DeathsWriteData = FightingVRPlayerState->GetDeaths();

#if TRACK_STATS_LOCALLY
							StatMatchesPlayed = (MatchWriteData += StatMatchesPlayed);
							StatKills = (KillsWriteData += StatKills);
							StatDeaths = (DeathsWriteData += StatDeaths);
#endif

							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SCORE, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_KILLS, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_DEATHS, DeathsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_MATCHESPLAYED, MatchWriteData);

							// the call will copy the user id and write object to its own memory
							Leaderboards->WriteLeaderboards(FightingVRPlayerState->SessionName, *UserId, ResultsWriteObject);
							Leaderboards->FlushLeaderboards(TEXT("FightingVR"));
						}
					}
				}
			}
		}
	}
}

void AFightingVRPlayerController::UpdateStatsOnGameEnd(bool bIsWinner)
{
	const IOnlineStatsPtr Stats = Online::GetStatsInterface(GetWorld());
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);

	if (Stats.IsValid() && LocalPlayer != nullptr && FightingVRPlayerState != nullptr)
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();

		if (UniqueId.IsValid() )
		{
			TArray<FOnlineStatsUserUpdatedStats> UpdatedUserStats;

			FOnlineStatsUserUpdatedStats& UpdatedStats = UpdatedUserStats.Emplace_GetRef( UniqueId.GetUniqueNetId().ToSharedRef() );
			UpdatedStats.Stats.Add( TEXT("Kills"), FOnlineStatUpdate( FightingVRPlayerState->GetKills(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("Deaths"), FOnlineStatUpdate( FightingVRPlayerState->GetDeaths(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("RoundsPlayed"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			if (bIsWinner)
			{
				UpdatedStats.Stats.Add( TEXT("RoundsWon"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			}

			Stats->UpdateStats( UniqueId.GetUniqueNetId().ToSharedRef(), UpdatedUserStats, FOnlineStatsUpdateStatsComplete() );
		}
	}
}


void AFightingVRPlayerController::UpdateSaveFileOnGameEnd(bool bIsWinner)
{
	AFightingVRPlayerState* FightingVRPlayerState = Cast<AFightingVRPlayerState>(PlayerState);
	if (FightingVRPlayerState)
	{
		// update local saved profile
		UFightingVRPersistentUser* const PersistentUser = GetPersistentUser();
		if (PersistentUser)
		{
			PersistentUser->AddMatchResult(FightingVRPlayerState->GetKills(), FightingVRPlayerState->GetDeaths(), FightingVRPlayerState->GetNumBulletsFired(), FightingVRPlayerState->GetNumRocketsFired(), bIsWinner);
			PersistentUser->SaveIfDirty();
		}
	}
}

void AFightingVRPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel( PendingURL, TravelType, bIsSeamlessTravel );

	if (const UWorld* World = GetWorld())
	{
		UFightingVRViewportClient* FightingVRViewport = Cast<UFightingVRViewportClient>( World->GetGameViewport() );

		if ( FightingVRViewport != NULL )
		{
			FightingVRViewport->ShowLoadingScreen();
		}
		
		AFightingVRHUD* FightingVRHUD = Cast<AFightingVRHUD>(GetHUD());
		if (FightingVRHUD != nullptr)
		{
			// Passing true to bFocus here ensures that focus is returned to the game viewport.
			FightingVRHUD->ShowScoreboard(false, true);
		}
	}
}
