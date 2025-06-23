// Copyright Epic Games, Inc. All Rights Reserved.

#include "TemplateGameMode.h"
#include "TemplateCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATemplateGameMode::ATemplateGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
