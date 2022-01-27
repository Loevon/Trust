// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrustPlayerController.generated.h"

UCLASS()
class ATrustPlayerController : public APlayerController {
	GENERATED_BODY()

public:
	ATrustPlayerController();

private:
	UPROPERTY()
	APawn *ControlledPawn;

	uint32 bMoveToMouseCursor : 1;

protected:
	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
	
	void MoveForward(float Value);
	
	void MoveRight(float Value);

	// UFUNCTION(Client, Reliable, BlueprintCallable)
	// void ClientShowNotification(const FText &Message);
	//
	// UFUNCTION(BlueprintImplementableEvent)
	// void ShowNotification(const FText &Message);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class ATrustCharacter *Killer);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const class UInventoryComponent *LootSource);
	
	UFUNCTION(BlueprintImplementableEvent)
	void ShowInGameUI();
	
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
};
