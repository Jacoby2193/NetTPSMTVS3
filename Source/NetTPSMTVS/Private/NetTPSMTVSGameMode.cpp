// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetTPSMTVSGameMode.h"
#include "NetTPSMTVSCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANetTPSMTVSGameMode::ANetTPSMTVSGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
