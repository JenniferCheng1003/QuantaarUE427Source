// Copyright Epic Games, Inc. All Rights Reserved.

#include "FightingVRCharacter.h"
#include "FightingVR.h"
#include "Weapons/FightingVRWeapon.h"
#include "Weapons/FightingVRDamageType.h"
#include "UI/FightingVRHUD.h"
#include "Online/FightingVRPlayerState.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundNodeLocalPlayer.h"
#include "AudioThread.h"

static int32 NetVisualizeRelevancyTestPoints = 0;
FAutoConsoleVariableRef CVarNetVisualizeRelevancyTestPoints(
	TEXT("p.NetVisualizeRelevancyTestPoints"),
	NetVisualizeRelevancyTestPoints,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);


static int32 NetEnablePauseRelevancy = 1;
FAutoConsoleVariableRef CVarNetEnablePauseRelevancy(
	TEXT("p.NetEnablePauseRelevancy"),
	NetEnablePauseRelevancy,
	TEXT("")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

FOnFightingVRCharacterEquipWeapon AFightingVRCharacter::NotifyEquipWeapon;
FOnFightingVRCharacterUnEquipWeapon AFightingVRCharacter::NotifyUnEquipWeapon;

AFightingVRCharacter::AFightingVRCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFightingVRCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	LowHealthPercentage = 0.5f;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
}

void AFightingVRCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetLocalRole() == ROLE_Authority)
	{
		Health = GetMaxHealth();

		// Needs to happen after character is added to repgraph
		GetWorldTimerManager().SetTimerForNextTick(this, &AFightingVRCharacter::SpawnDefaultInventory);
	}

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();

	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void AFightingVRCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AFightingVRCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);
}

void AFightingVRCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void AFightingVRCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (GetPlayerState() != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator AFightingVRCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AFightingVRCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AFightingVRPlayerState* TestPlayerState = Cast<AFightingVRPlayerState>(TestPC->PlayerState);
	AFightingVRPlayerState* MyPlayerState = Cast<AFightingVRPlayerState>(GetPlayerState());

	bool bIsEnemy = true;
	if (GetWorld()->GetGameState())
	{
		const AFightingVRMode* DefGame = GetWorld()->GetGameState()->GetDefaultGameMode<AFightingVRMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}

//////////////////////////////////////////////////////////////////////////
// Meshes

void AFightingVRCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	Mesh1P->VisibilityBasedAnimTickOption = !bFirstPerson ? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered : EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->VisibilityBasedAnimTickOption = bFirstPerson ? EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered : EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AFightingVRCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AFightingVRPlayerState* MyPlayerState = Cast<AFightingVRPlayerState>(GetPlayerState());
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

void AFightingVRCharacter::OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->GetRelativeRotation(), DefMesh1P->GetRelativeLocation());
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	Mesh1P->SetRelativeLocationAndRotation(PitchedMesh.GetOrigin(), PitchedMesh.Rotator());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void AFightingVRCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AFightingVRCharacter::Suicide()
{
	KilledBy(this);
}

void AFightingVRCharacter::KilledBy(APawn* EventInstigator)
{
	if (GetLocalRole() == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}


float AFightingVRCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	AFightingVRMode* const Game = GetWorld()->GetAuthGameMode<AFightingVRMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}


bool AFightingVRCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| GetLocalRole() != ROLE_Authority				// not authority
		|| GetWorld()->GetAuthGameMode<AFightingVRMode>() == NULL
		|| GetWorld()->GetAuthGameMode<AFightingVRMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AFightingVRCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AFightingVRMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AFightingVRCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void AFightingVRCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	SetReplicatingMovement(false);
	TearOff();
	bIsDying = true;

	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		AFightingVRPlayerController* PC = Cast<AFightingVRPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UFightingVRDamageType *DamageType = Cast<UFightingVRDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, FFParams);
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();

	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Trigger ragdoll a little before the animation early so the character doesn't
		// blend back to its normal position.
		const float TriggerRagdollTime = DeathAnimDuration - 0.7f;

		// Enable blend physics so the bones are properly blending against the montage.
		GetMesh()->bBlendPhysics = true;

		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AFightingVRCharacter::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	}
	else
	{
		SetRagdollPhysics();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AFightingVRCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		AFightingVRPlayerController* PC = Cast<AFightingVRPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UFightingVRDamageType *DamageType = Cast<UFightingVRDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, FFParams);
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}

	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	AFightingVRHUD* MyHUD = MyPC ? Cast<AFightingVRHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyWeaponHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AFightingVRPlayerController* InstigatorPC = Cast<AFightingVRPlayerController>(PawnInstigator->Controller);
		AFightingVRHUD* InstigatorHUD = InstigatorPC ? Cast<AFightingVRHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}
}


void AFightingVRCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}



void AFightingVRCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AFightingVRCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AFightingVRCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AFightingVRCharacter::TornOff()
{
	SetLifeSpan(25.f);
}

bool AFightingVRCharacter::IsMoving()
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AFightingVRCharacter::SpawnDefaultInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AFightingVRWeapon* NewWeapon = GetWorld()->SpawnActor<AFightingVRWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void AFightingVRCharacter::DestroyInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AFightingVRWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void AFightingVRCharacter::AddWeapon(AFightingVRWeapon* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void AFightingVRCharacter::RemoveWeapon(AFightingVRWeapon* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

AFightingVRWeapon* AFightingVRCharacter::FindWeapon(TSubclassOf<AFightingVRWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void AFightingVRCharacter::EquipWeapon(AFightingVRWeapon* Weapon)
{
	if (Weapon)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon, CurrentWeapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AFightingVRCharacter::ServerEquipWeapon_Validate(AFightingVRWeapon* Weapon)
{
	return true;
}

void AFightingVRCharacter::ServerEquipWeapon_Implementation(AFightingVRWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AFightingVRCharacter::OnRep_CurrentWeapon(AFightingVRWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AFightingVRCharacter::SetCurrentWeapon(AFightingVRWeapon* NewWeapon, AFightingVRWeapon* LastWeapon)
{
	AFightingVRWeapon* LocalLastWeapon = nullptr;

	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!

		NewWeapon->OnEquip(LastWeapon);
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AFightingVRCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AFightingVRCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AFightingVRCharacter::CanFire() const
{
	return IsAlive();
}

bool AFightingVRCharacter::CanReload() const
{
	return true;
}

void AFightingVRCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::SpawnSoundAttached(TargetingSound, GetRootComponent());
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AFightingVRCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AFightingVRCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AFightingVRCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}
}

bool AFightingVRCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AFightingVRCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AFightingVRCharacter::UpdateRunSounds()
{
	const bool bIsRunSoundPlaying = RunLoopAC != nullptr && RunLoopAC->IsActive();
	const bool bWantsRunSoundPlaying = IsRunning() && IsMoving();

	// Don't bother playing the sounds unless we're running and moving.
	if (!bIsRunSoundPlaying && bWantsRunSoundPlaying)
	{
		if (RunLoopAC != nullptr)
		{
			RunLoopAC->Play();
		}
		else if (RunLoopSound != nullptr)
		{
			RunLoopAC = UGameplayStatics::SpawnSoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC != nullptr)
			{
				RunLoopAC->bAutoDestroy = false;
			}
		}
	}
	else if (bIsRunSoundPlaying && !bWantsRunSoundPlaying)
	{
		RunLoopAC->Stop();
		if (RunStopSound != nullptr)
		{
			UGameplayStatics::SpawnSoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AFightingVRCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AFightingVRCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime(), AnimMontage);
	}
}

void AFightingVRCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void AFightingVRCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AFightingVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFightingVRCharacter::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &AFightingVRCharacter::MoveUp);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AFightingVRCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AFightingVRCharacter::LookUpAtRate);

	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC->bAnalogFireTrigger)
	{
		PlayerInputComponent->BindAxis("FireTrigger", this, &AFightingVRCharacter::FireTrigger);
	}
	else
	{
		PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFightingVRCharacter::OnStartFire);
		PlayerInputComponent->BindAction("Fire", IE_Released, this, &AFightingVRCharacter::OnStopFire);
	}

	PlayerInputComponent->BindAction("Targeting", IE_Pressed, this, &AFightingVRCharacter::OnStartTargeting);
	PlayerInputComponent->BindAction("Targeting", IE_Released, this, &AFightingVRCharacter::OnStopTargeting);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &AFightingVRCharacter::OnNextWeapon);
	PlayerInputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AFightingVRCharacter::OnPrevWeapon);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AFightingVRCharacter::OnReload);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFightingVRCharacter::OnStartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AFightingVRCharacter::OnStopJump);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AFightingVRCharacter::OnStartRunning);
	PlayerInputComponent->BindAction("RunToggle", IE_Pressed, this, &AFightingVRCharacter::OnStartRunningToggle);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AFightingVRCharacter::OnStopRunning);
}


void AFightingVRCharacter::FireTrigger(float Val)
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (bWantsToFire && Val < MyPC->FireTriggerThreshold)
	{
		OnStopFire();
	}
	else if (!bWantsToFire && Val >= MyPC->FireTriggerThreshold)
	{
		OnStartFire();
	}
}

void AFightingVRCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		AddMovementInput(Direction, Val);
	}
}

void AFightingVRCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FQuat Rotation = GetActorQuat();
		const FVector Direction = FQuatRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
		AddMovementInput(Direction, Val);
	}
}

void AFightingVRCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void AFightingVRCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AFightingVRCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AFightingVRCharacter::OnStartFire()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void AFightingVRCharacter::OnStopFire()
{
	StopWeaponFire();
}

void AFightingVRCharacter::OnStartTargeting()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void AFightingVRCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void AFightingVRCharacter::OnNextWeapon()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AFightingVRWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void AFightingVRCharacter::OnPrevWeapon()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AFightingVRWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void AFightingVRCharacter::OnReload()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AFightingVRCharacter::OnStartRunning()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, false);
	}
}

void AFightingVRCharacter::OnStartRunningToggle()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, true);
	}
}

void AFightingVRCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

bool AFightingVRCharacter::IsRunning() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}

void AFightingVRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health += 5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}

	if (GEngine->UseSound())
	{
		if (LowHealthSound)
		{
			if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
			{
				LowHealthWarningPlayer = UGameplayStatics::SpawnSoundAttached(LowHealthSound, GetRootComponent(),
					NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
				if (LowHealthWarningPlayer)
				{
					LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
				}
			}
			else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				LowHealthWarningPlayer->Stop();
			}
			if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
			{
				const float MinVolume = 0.3f;
				const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
				LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
			}
		}

		UpdateRunSounds();
	}

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bLocallyControlled = (PC ? PC->IsLocalController() : false);
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
	    USoundNodeLocalPlayer::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
	
	TArray<FVector> PointsToTest;
	BuildPauseReplicationCheckPoints(PointsToTest);

	if (NetVisualizeRelevancyTestPoints == 1)
	{
		for (FVector PointToTest : PointsToTest)
		{
			DrawDebugSphere(GetWorld(), PointToTest, 10.0f, 8, FColor::Red);
		}
	}
}

void AFightingVRCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			USoundNodeLocalPlayer::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void AFightingVRCharacter::OnStartJump()
{
	AFightingVRPlayerController* MyPC = Cast<AFightingVRPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void AFightingVRCharacter::OnStopJump()
{
	bPressedJump = false;
	StopJumping();
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AFightingVRCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(AFightingVRCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout);
}

void AFightingVRCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(AFightingVRCharacter, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AFightingVRCharacter, bIsTargeting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AFightingVRCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AFightingVRCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AFightingVRCharacter, CurrentWeapon);
	DOREPLIFETIME(AFightingVRCharacter, Health);
}

bool AFightingVRCharacter::IsReplicationPausedForConnection(const FNetViewer& ConnectionOwnerNetViewer)
{
	if (NetEnablePauseRelevancy == 1)
	{
		APlayerController* PC = Cast<APlayerController>(ConnectionOwnerNetViewer.InViewer);
		check(PC);

		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(LineOfSight), true, PC->GetPawn());
		CollisionParams.AddIgnoredActor(this);

		TArray<FVector> PointsToTest;
		BuildPauseReplicationCheckPoints(PointsToTest);

		for (FVector PointToTest : PointsToTest)
		{
			if (!GetWorld()->LineTraceTestByChannel(PointToTest, ViewLocation, ECC_Visibility, CollisionParams))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void AFightingVRCharacter::OnReplicationPausedChanged(bool bIsReplicationPaused)
{
	GetMesh()->SetHiddenInGame(bIsReplicationPaused, true);
}

AFightingVRWeapon* AFightingVRCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 AFightingVRCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AFightingVRWeapon* AFightingVRCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* AFightingVRCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* AFightingVRCharacter::GetSpecifcPawnMesh(bool WantFirstPerson) const
{
	return WantFirstPerson == true ? Mesh1P : GetMesh();
}

FName AFightingVRCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float AFightingVRCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool AFightingVRCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float AFightingVRCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool AFightingVRCharacter::IsFiring() const
{
	return bWantsToFire;
};

bool AFightingVRCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController();
}

int32 AFightingVRCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AFightingVRCharacter>()->Health;
}

bool AFightingVRCharacter::IsAlive() const
{
	return Health > 0;
}

float AFightingVRCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AFightingVRCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}

void AFightingVRCharacter::BuildPauseReplicationCheckPoints(TArray<FVector>& RelevancyCheckPoints)
{
	FBoxSphereBounds Bounds = GetCapsuleComponent()->CalcBounds(GetCapsuleComponent()->GetComponentTransform());
	FBox BoundingBox = Bounds.GetBox();
	float XDiff = Bounds.BoxExtent.X * 2;
	float YDiff = Bounds.BoxExtent.Y * 2;

	RelevancyCheckPoints.Add(BoundingBox.Min);
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(BoundingBox.Max);
}