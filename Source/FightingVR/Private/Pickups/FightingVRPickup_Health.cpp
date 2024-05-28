// Copyright Epic Games, Inc. All Rights Reserved.

#include "Pickups/FightingVRPickup_Health.h"
#include "FightingVR.h"
#include "OnlineSubsystemUtils.h"

AFightingVRPickup_Health::AFightingVRPickup_Health(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Health = 50;
}

bool AFightingVRPickup_Health::CanBePickedUp(class AFightingVRCharacter* TestPawn) const
{
	return TestPawn && (TestPawn->Health < TestPawn->GetMaxHealth());
}

void AFightingVRPickup_Health::GivePickupTo(class AFightingVRCharacter* Pawn)
{
	if (Pawn)
	{
		Pawn->Health = FMath::Min(FMath::TruncToInt(Pawn->Health) + Health, Pawn->GetMaxHealth());

		// Fire event for collected health
		const UWorld* World = GetWorld();
		const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
		const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

		if (Events.IsValid() && Identity.IsValid())
		{							
			AFightingVRPlayerController* PC = Cast<AFightingVRPlayerController>(Pawn->Controller);
			if (PC)
			{
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player);

				if (LocalPlayer)
				{
					const int32 UserIndex = LocalPlayer->GetControllerId();
					TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);			
					if (UniqueID.IsValid())
					{
						FVector Location = Pawn->GetActorLocation();

						FOnlineEventParms Params;		

						Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
						Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
						Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

						Params.Add( TEXT( "ItemId" ), FVariantData( (int32)0 ) ); // @todo come up with a better way to determine item id, currently health is 0 and ammo counts from 1
						Params.Add( TEXT( "AcquisitionMethodId" ), FVariantData( (int32)0 ) ); // unused
						Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
						Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
						Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
						Params.Add( TEXT( "ItemQty" ), FVariantData( (int32)Health ) );			

						Events->TriggerEvent(*UniqueID, TEXT("CollectPowerup"), Params);
					}
				}
			}
		}
	}
}
