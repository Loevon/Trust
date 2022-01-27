// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrustPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "TrustCharacter.h"
#include "Engine/World.h"

ATrustPlayerController::ATrustPlayerController() {
	bShowMouseCursor = true;
	
	DefaultMouseCursor = EMouseCursor::Default;
}

void ATrustPlayerController::PlayerTick(float DeltaTime) {
	Super::PlayerTick(DeltaTime);

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor) {
		MoveToMouseCursor();
	}
}

void ATrustPlayerController::SetupInputComponent() {
	Super::SetupInputComponent();

	InputComponent->BindAxis("MoveRight", this, &ATrustPlayerController::MoveRight);
	InputComponent->BindAxis("MoveForward", this, &ATrustPlayerController::MoveForward);
	InputComponent->BindAction("SetDestination", IE_Pressed, this, &ATrustPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &ATrustPlayerController::OnSetDestinationReleased);
}

void ATrustPlayerController::MoveToMouseCursor() {
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit) {
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

void ATrustPlayerController::SetNewMoveDestination(const FVector DestLocation) {
	// APawn* ControlledPawn = GetPawn();
	if (ControlledPawn) {
		float const Distance = FVector::Dist(DestLocation, ControlledPawn->GetActorLocation());

		// We need to issue move command only if far enough in order for walk animation to play correctly
		if ((Distance > 120.0f)) {
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
		}
	}
}

void ATrustPlayerController::OnSetDestinationPressed() {
	bMoveToMouseCursor = true;
}

void ATrustPlayerController::OnSetDestinationReleased() {
	bMoveToMouseCursor = false;
}

void ATrustPlayerController::MoveForward(float Value) {
	// APawn* ControlledPawn = GetPawn();
	if ((ControlledPawn != nullptr) && (Value != 0.0f)) {
		//	bMoveToMouseCursor = false;
		// find out which way is forward
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		ControlledPawn->AddMovementInput(Direction, Value);
	}
}

void ATrustPlayerController::MoveRight(float Value) {
	// APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr) {
		ControlledPawn = GetPawn();
	}
	
	if ( (ControlledPawn != nullptr) && (Value != 0.0f) ) {
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		ControlledPawn->AddMovementInput(Direction, Value);
	}
}

