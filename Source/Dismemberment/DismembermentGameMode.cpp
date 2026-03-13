// Copyright Epic Games, Inc. All Rights Reserved.

#include "DismembermentGameMode.h"
#include "DismembermentCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADismembermentGameMode::ADismembermentGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
