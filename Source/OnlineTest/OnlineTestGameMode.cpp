// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineTestGameMode.h"
#include "OnlineTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

AOnlineTestGameMode::AOnlineTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
