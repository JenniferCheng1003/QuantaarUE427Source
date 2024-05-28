// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Net/UnrealNetwork.h"
#include "FightingVRMode.h"
#include "FightingVRState.h"
#include "FightingVRCharacter.h"
#include "FightingVRCharacterMovement.h"
#include "FightingVRPlayerController.h"
#include "FightingVRClasses.h"


class UBehaviorTreeComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogFightingVR, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogFightingVRWeapon, Log, All);

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define MAX_PLAYER_NAME_LENGTH 16


#ifndef FIGHTINGVR_CONSOLE_UI
/** Set to 1 to pretend we're building for console even on a PC, for testing purposes */
#define FIGHTINGVR_SIMULATE_CONSOLE_UI	0

#if PLATFORM_PS4 || PLATFORM_SWITCH || FIGHTINGVR_SIMULATE_CONSOLE_UI
	#define FIGHTINGVR_CONSOLE_UI 1
#else
	#define FIGHTINGVR_CONSOLE_UI 0
#endif
#endif

#ifndef FIGHTINGVR_XBOX_STRINGS
	#define FIGHTINGVR_XBOX_STRINGS 0
#endif

#ifndef FIGHTINGVR_SHOW_QUIT_MENU_ITEM
	#define FIGHTINGVR_SHOW_QUIT_MENU_ITEM (!FIGHTINGVR_CONSOLE_UI)
#endif

#ifndef FIGHTINGVR_SUPPORTS_OFFLINE_SPLIT_SCREEEN
	#define FIGHTINGVR_SUPPORTS_OFFLINE_SPLIT_SCREEEN 1
#endif

// whether the platform will signal a controller pairing change on a controller disconnect. if not, we need to treat the pairing change as a request to switch profiles when the destination profile is not specified
#ifndef FIGHTINGVR_CONTROLLER_PAIRING_ON_DISCONNECT
	#define FIGHTINGVR_CONTROLLER_PAIRING_ON_DISCONNECT 1
#endif

// whether the game should display an account picker when a new input device is connected, while the "please reconnect controller" message is on screen.
#ifndef FIGHTINGVR_CONTROLLER_PAIRING_PROMPT_FOR_NEW_USER_WHEN_RECONNECTING
	#define FIGHTINGVR_CONTROLLER_PAIRING_PROMPT_FOR_NEW_USER_WHEN_RECONNECTING 0
#endif
