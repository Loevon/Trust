// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Items/EquippableItem.h"
#include "TrustCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippedItemsChanged, const EEquippableSlot, Slot, const UEquippableItem*, Item);

USTRUCT()
struct FInteractionData {
	GENERATED_BODY()
	
	FInteractionData() {
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.f;
		bInteractionHeld = false;
	}
	
	UPROPERTY()
	class UInteractionComponent *ViewedInteractionComponent;

	UPROPERTY()
	float LastInteractionCheckTime;

	UPROPERTY()
	bool bInteractionHeld;
};

UCLASS(Blueprintable)
class ATrustCharacter : public ACharacter {
	GENERATED_BODY()

public:
	ATrustCharacter();

private:
	uint32 bIsCombatMode;
	
	uint32 bIsAiming;

	FTimerHandle TimerHandle_Interact;

	UPROPERTY()
	FInteractionData InteractionData;
	
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float InteractionCheckFrequency;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float InteractionCheckDistance;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent *PlayerInventory;

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TSubclassOf<class APickup> PickupClass;

	UPROPERTY(VisibleAnywhere, Category = "Items")
    TMap<EEquippableSlot, UEquippableItem*> EquippedItems;

	UPROPERTY(BlueprintAssignable, Category = "Items")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	UPROPERTY(BlueprintReadOnly, Category = Mesh)
    TMap<EEquippableSlot, USkeletalMesh*> NakedMeshes;

    UPROPERTY(BlueprintReadOnly, Category = Mesh)
    TMap<EEquippableSlot, USkeletalMeshComponent*> PlayerMeshes;

	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent *HeadMesh;
    
	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent *ChestMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent *LegsMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent *FeetMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	USkeletalMeshComponent *HandsMesh;
	
	// UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
 //    class AWeapon *EquippedWeapon;
    

	
public:
	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	void ToggleCombat();

	void ToggleAiming();

	bool IsInteracting() const;

	float GetRemainingInteractTime() const;
	
	FORCEINLINE UInventoryComponent* GetPlayerInventory() const { return PlayerInventory; }
	
	UFUNCTION(BlueprintCallable, Category = "Items")
    void UseItem(class UItem *Item);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerUseItem(class UItem *Item);

    UFUNCTION(BlueprintCallable, Category = "Items")
    void DropItem(class UItem *Item, const int32 Quantity);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerDropItem(class UItem *Item, const int32 Quantity);
	
	// equipment items
	bool EquipItem(UEquippableItem *Item);
	
    bool UnEquipItem(UEquippableItem *Item);

    void EquipArmor(const class UArmorItem *Armor);
	
    void UnEquipArmor(const EEquippableSlot Slot);

    // void EquipWeapon(class UWeaponItem *WeaponItem);
    // void UnEquipWeapon();
	
    UFUNCTION(BlueprintPure)
	USkeletalMeshComponent *GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);

	UFUNCTION(BlueprintPure)
    FORCEINLINE TMap<EEquippableSlot, UEquippableItem*> GetEquippedItems() const { return EquippedItems; }

    // UFUNCTION(BlueprintPure, Category = "Weapons")
    // FORCEINLINE class AWeapon *GetEquippedWeapon() const { return EquippedWeapon; }

private:
	void SetRotation(FRotator NewRotation);

	UFUNCTION(Server, Reliable)
	void Server_SetRotation(FRotator NewRotation);

	FVector GetMousePosition() const;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnCombatModeToggled(bool bCombat);

	UFUNCTION(BlueprintImplementableEvent)
	void OnAimToggled(bool bAim);

	void Interact();

	void EndInteract();
	
	void BeginInteraction();
	
	void BeginInteraction(FVector MousePos);
	
	void PerformInteractionCheck(FVector MousePos);

	void CouldntFindInteractable();

    void FoundNewInteractable(UInteractionComponent *Interactable);

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	FORCEINLINE class UInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent; }

	UFUNCTION(Server, Reliable, WithValidation)
    void ServerBeginInteraction(FVector MousePos);

	UFUNCTION(Server, Reliable, WithValidation)
    void ServerEndInteraction();

	// UFUNCTION()
	// void OnRep_EquippedWeapon();
};
