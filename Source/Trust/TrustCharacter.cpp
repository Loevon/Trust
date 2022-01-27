// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrustCharacter.h"

#include "DrawDebugHelpers.h"
#include "TrustPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/ArmorItem.h"
#include "World/Pickup.h"

ATrustCharacter::ATrustCharacter() {
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
// Trust	
	HeadMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Head, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh")));
    ChestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Chest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
    LegsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Legs, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
    FeetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Feet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
    HandsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Hands, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));

	for (const auto &PlayerMesh : PlayerMeshes) {
    	USkeletalMeshComponent *MeshComponent = PlayerMesh.Value;
    	MeshComponent->SetupAttachment(GetMesh());
    	MeshComponent->SetMasterPoseComponent(GetMesh());
    }
	
	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
    PlayerInventory->SetCapacity(20);
    PlayerInventory->SetWeightCapacity(80.f);
	
	InteractionCheckDistance = 5000.f;
    InteractionCheckFrequency = 0.f;

	bIsAiming = false;
	bIsCombatMode = false;
}

void ATrustCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("ToggleCombat", IE_Pressed, this, &ATrustCharacter::ToggleCombat);
	PlayerInputComponent->BindAction("ToggleAim", IE_Pressed, this, &ATrustCharacter::ToggleAiming);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ATrustCharacter::BeginInteraction);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ATrustCharacter::EndInteract);
}

void ATrustCharacter::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
	
	// if (bIsCombatMode && CurrentPlayerController) {
	// 	FHitResult TraceHitResult;
	// 	CurrentPlayerController->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
	// 	
	// 	FRotator PlayerRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TraceHitResult.Location);
	// 	FRotator NewRotation = GetActorRotation();
	// 	NewRotation.Yaw = PlayerRot.Yaw;
	// 	SetRotation(NewRotation);
	// 	// UE_LOG(LogLevel, Warning, TEXT("Mouse loc: %s"), *NewRotation.ToString());
	// }

	
	// const bool bIsInteractingOnServer = (HasAuthority() && IsInteracting());
	// || bIsInteractingOnServer
	if (!HasAuthority() && GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency) {
		if (Cast<ATrustPlayerController>(GetController())) {
			const FVector MousePos = GetMousePosition();
			PerformInteractionCheck(MousePos);
		}
	}
}

void ATrustCharacter::ToggleCombat() {
	bIsCombatMode = !bIsCombatMode;
	OnCombatModeToggled(bIsCombatMode);
}

void ATrustCharacter::ToggleAiming() {
	bIsAiming = !bIsAiming;
	OnAimToggled(bIsAiming);
}

void ATrustCharacter::SetRotation(FRotator NewRotation) {
	if (!HasAuthority()) {
		Server_SetRotation(NewRotation);
	}

	SetActorRotation(NewRotation);
}

void ATrustCharacter::Server_SetRotation_Implementation(FRotator NewRotation) {
	SetRotation(NewRotation);
}

void ATrustCharacter::Interact() {
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

    if (UInteractionComponent* Interactable = GetInteractable()) {
    	Interactable->Interact(this);
    }
}

void ATrustCharacter::BeginInteraction() {
	const FVector MousePos = GetMousePosition();

	if (!HasAuthority()) {
    	ServerBeginInteraction(MousePos);
    }

	BeginInteraction(MousePos);
}

void ATrustCharacter::BeginInteraction(FVector MousePos) {
	if (HasAuthority()) {
		PerformInteractionCheck(MousePos);
	}
	
	InteractionData.bInteractionHeld = true;
	
	if (UInteractionComponent* Interactable = GetInteractable()) {
		Interactable->BeginInteract(this);

		if (FMath::IsNearlyZero(Interactable->GetInteractionTime())) {
			Interact();
		} else {
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &ATrustCharacter::Interact, Interactable->GetInteractionTime(), false);
		}
	}
}

void ATrustCharacter::ServerBeginInteraction_Implementation(FVector MousePos) {
	BeginInteraction(MousePos);
}

bool ATrustCharacter::ServerBeginInteraction_Validate(FVector MousePos) {
	return true;
}


void ATrustCharacter::EndInteract() {
	if (!HasAuthority()) {
		ServerEndInteraction();
	}

	InteractionData.bInteractionHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (UInteractionComponent* Interactable = GetInteractable()) {
		Interactable->EndInteract(this);
	}	
}

void ATrustCharacter::ServerEndInteraction_Implementation() {
	EndInteract();
}

bool ATrustCharacter::ServerEndInteraction_Validate() {
	return true;
}

bool ATrustCharacter::IsInteracting() const {
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}

float ATrustCharacter::GetRemainingInteractTime() const {
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}

void ATrustCharacter::PerformInteractionCheck(FVector MousePos) {
	if (GetController() == nullptr) {
    	return;
    }

	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector Start = GetActorLocation();
	FVector InteractVector = MousePos - Start;
	FVector End = Start + (InteractVector.GetSafeNormal() * InteractionCheckDistance);

	FHitResult TraceHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	// DrawDebugLine(GetWorld(), Start, End, FColor::Purple, false, 2.f);
	if (GetWorld()->LineTraceSingleByChannel(TraceHit, Start, End, ECC_Visibility, QueryParams)) {
		AActor* TraceHitActor = TraceHit.GetActor();
		if (TraceHitActor) {
			// UE_LOG(LogLevel, Warning, TEXT("Found: %s"), *TraceHitActor->GetName());

			UActorComponent *Comp = TraceHitActor->GetComponentByClass(UInteractionComponent::StaticClass());
			UInteractionComponent* InteractionComponent = Cast<UInteractionComponent>(Comp);
			
			if (InteractionComponent) {
				
            	float Distance = (Start - TraceHit.ImpactPoint).Size();
            	
            	if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->GetInteractionDistance()) {
            		FoundNewInteractable(InteractionComponent);
            	} else if (Distance > InteractionComponent->GetInteractionDistance() && GetInteractable()) {
            		CouldntFindInteractable();
            	}

				return;
            }
		}
	}

	CouldntFindInteractable();
}

void ATrustCharacter::CouldntFindInteractable() {
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact)) {
    	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
    }

    //Tell the interactable we've stopped focusing on it, and clear the current interactable
    if (UInteractionComponent* Interactable = GetInteractable()) {
    	Interactable->EndFocus(this);

    	if (InteractionData.bInteractionHeld) {
    		EndInteract();
    	}
    }

    InteractionData.ViewedInteractionComponent = nullptr;
}

void ATrustCharacter::FoundNewInteractable(UInteractionComponent* Interactable) {
	// UE_LOG(LogTemp, Warning, TEXT("We found an interactable!"));
    
    EndInteract();

    if (UInteractionComponent* OldInteractable = GetInteractable()) {
    	OldInteractable->EndFocus(this);
    }

    InteractionData.ViewedInteractionComponent = Interactable;
    Interactable->BeginFocus(this);
}

FVector ATrustCharacter::GetMousePosition() const {
	FHitResult CursorTraceHit;
	const ATrustPlayerController* CurrentPlayerController = Cast<ATrustPlayerController>(GetController());
	CurrentPlayerController->GetHitResultUnderCursor(ECC_Visibility, true, CursorTraceHit);
	return CursorTraceHit.Location;
}

void ATrustCharacter::UseItem(UItem* Item) {
	if (!HasAuthority() && Item) {
		ServerUseItem(Item);
	}
	
	if (HasAuthority()) {
		if (PlayerInventory && !PlayerInventory->FindItem(Item)) {
			return;
		}
	}

	if (Item) {
		Item->OnUse(this);
		Item->Use(this);
	}
}

void ATrustCharacter::DropItem(UItem* Item, const int32 Quantity) {
	if (PlayerInventory && Item && PlayerInventory->FindItem(Item)) {
		if (!HasAuthority()) {
			ServerDropItem(Item, Quantity);
			return;
		}

		if (HasAuthority()) {
			const int32 ItemQuantity = Item->GetQuantity();
			const int32 DroppedQuantity = PlayerInventory->ConsumeItem(Item, Quantity);
			
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.bNoFail = true;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLocation = GetActorLocation();
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			ensure(PickupClass);

			APickup *Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParameters);
			Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
		}
	}
}

bool ATrustCharacter::EquipItem(UEquippableItem* Item) {
	EquippedItems.Add(Item->GetSlot(), Item);
	OnEquippedItemsChanged.Broadcast(Item->GetSlot(), Item);
	return true;
}

bool ATrustCharacter::UnEquipItem(UEquippableItem* Item) {
	if (Item) {
		if (EquippedItems.Contains(Item->GetSlot())) {
			if (Item == *EquippedItems.Find(Item->GetSlot())) {
				EquippedItems.Remove(Item->GetSlot());
				OnEquippedItemsChanged.Broadcast(Item->GetSlot(), nullptr);
				return true;
			}
		}
	}
	return false;
}

void ATrustCharacter::EquipArmor(const UArmorItem* Armor) {
	if (USkeletalMeshComponent *GearMesh = *PlayerMeshes.Find(Armor->GetSlot())) {
		GearMesh->SetSkeletalMesh(Armor->GetMesh());
		GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Armor->GetMaterialInstance());
	}
}

void ATrustCharacter::UnEquipArmor(const EEquippableSlot Slot) {
	if (USkeletalMeshComponent *EquippableMesh = *PlayerMeshes.Find(Slot)) {
		EquippableMesh->SetSkeletalMesh(nullptr);
		// if (USkeletalMesh *BodyMesh = *NakedMeshes.Find(Slot)) {
		// 	EquippableMesh->SetSkeletalMesh(BodyMesh);
		//
		// 	for (int32 i = 0; i < BodyMesh->GetMaterials().Num(); ++i) {
		// 		if (BodyMesh->GetMaterials().IsValidIndex(i)) {
		// 			EquippableMesh->SetMaterial(i, BodyMesh->GetMaterials()[i].MaterialInterface);
		// 		}
		// 	}
		// } else {
		// 	EquippableMesh->SetSkeletalMesh(nullptr);
		// }
	}
}

USkeletalMeshComponent* ATrustCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot) {
	if (PlayerMeshes.Contains(Slot)) {
		return *PlayerMeshes.Find(Slot);
	}
	return nullptr;
}

void ATrustCharacter::ServerDropItem_Implementation(UItem* Item, int32 Quantity) {
	DropItem(Item, Quantity);
}

void ATrustCharacter::ServerUseItem_Implementation(UItem* Item) {
	UseItem(Item);
}

bool ATrustCharacter::ServerDropItem_Validate(UItem* Item, int32 Quantity) {
	return true;
}

bool ATrustCharacter::ServerUseItem_Validate(UItem* Item) {
	return true;
}
