// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trust/Public/Items/Item.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"


//Called when the inventory is changed and the UI needs an update. 
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

/**Called on server when an item is added to this inventory*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, class UItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, class UItem*, Item);

// TODO: Move to own file
UENUM(BlueprintType)
enum class EItemAddResult : uint8 {
	IAR_NoItemsAdded UMETA(DisplayName = "No items added"),
	IAR_SomeItemsAdded UMETA(DisplayName = "Some items added"),
	IAR_AllItemsAdded UMETA(DisplayName = "All items added")
};

//Represents the result of adding an item to the inventory.
USTRUCT(BlueprintType)
struct FItemAddResult {

	GENERATED_BODY()

public:
	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), AmountGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), AmountGiven(InQuantityAdded) {};

	//The amount of the item that we tried to add
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountToGive = 0;

	//The amount of the item that was actually added in the end. Maybe we tried adding 10 items, but only 8 could be added because of capacity/weight
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountGiven = 0;

	//The result
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult Result = EItemAddResult::IAR_NoItemsAdded;

	//If something went wrong, like we didnt have enough capacity or carrying too much weight this contains the reason why
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ErrorText = FText::GetEmpty();

	//Helpers
	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText) {
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	static FItemAddResult AddedSome(const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText) {
		FItemAddResult AddedSomeResult(InItemQuantity, ActualAmountGiven);

		AddedSomeResult.Result = EItemAddResult::IAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;

		return AddedSomeResult;
	}

	static FItemAddResult AddedAll(const int32 InItemQuantity) {
		FItemAddResult AddAllResult(InItemQuantity, InItemQuantity);

		AddAllResult.Result = EItemAddResult::IAR_AllItemsAdded;

		return AddAllResult;
	}
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TRUST_API UInventoryComponent : public UActorComponent {
	GENERATED_BODY()

public:	
	UInventoryComponent();

private:
	UPROPERTY()
	int32 ReplicatedItemsKey;

protected:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnItemRemoved OnItemRemoved;

	//The maximum weight the inventory can hold. For players, backpacks and other items increase this limit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    float WeightCapacity;

    //The maximum number of items the inventory can hold. For players, backpacks and other items increase this limit
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0, ClampMax = 200))
    int32 Capacity;

    /**The items currently in our inventory*/
    UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, Category = "Inventory")
    TArray<UItem*> Items;

	UPROPERTY()
    TArray<UItem*> ClientLastReceivedItems;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
    FItemAddResult TryAddItem(UItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
    FItemAddResult TryAddItemFromClass(TSubclassOf<UItem> ItemClass, const int32 Quantity = 1);

	int32 ConsumeItem(UItem* Item);
	
	int32 ConsumeItem(UItem* Item, const int32 Quantity);

	void MarkDirtyForReplication() { ReplicatedItemsKey++; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItem* Item);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(TSubclassOf <UItem> ItemClass, const int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItem* FindItem(UItem* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItem* FindItemByClass(TSubclassOf<class UItem> ItemClass) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<UItem*> FindItemsByClass(TSubclassOf<class UItem> ItemClass) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
    float GetCurrentWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetWeightCapacity(const float NewWeightCapacity);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void SetCapacity(const int32 NewCapacity);

    UFUNCTION(BlueprintPure, Category = "Inventory")
    FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };

    UFUNCTION(BlueprintPure, Category = "Inventory")
    FORCEINLINE int32 GetCapacity() const { return Capacity; }

    UFUNCTION(BlueprintPure, Category = "Inventory")
    FORCEINLINE TArray<UItem*> GetInventoryItems() const { return Items; }

    UFUNCTION(Client, Reliable)
    void ClientRefreshInventory();

private:
	UFUNCTION()
    void ItemAdded(UItem* Item);

    UFUNCTION()
    void ItemRemoved(UItem* Item);
	
	UItem* AddItem(UItem* Item, const int32 Quantity);

	FItemAddResult TryAddItem_Internal(UItem* Item);
	
	UFUNCTION()
    void OnRep_Items();
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool ReplicateSubobjects(UActorChannel *Channel, FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
};
