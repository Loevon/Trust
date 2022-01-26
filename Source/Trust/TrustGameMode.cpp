// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrustGameMode.h"
#include "TrustPlayerController.h"
#include "TrustCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATrustGameMode::ATrustGameMode() {
	// use our custom PlayerController class
	PlayerControllerClass = ATrustPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != nullptr) {
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}