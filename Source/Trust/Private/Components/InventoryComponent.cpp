// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "Inventory"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent() {
	PrimaryComponentTick.bCanEverTick = true;

	OnItemAdded.AddDynamic(this, &UInventoryComponent::ItemAdded);
    OnItemRemoved.AddDynamic(this, &UInventoryComponent::ItemRemoved);

    SetIsReplicatedByDefault(true);
}

FItemAddResult UInventoryComponent::TryAddItem(UItem* Item) {
	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<UItem> ItemClass, const int32 Quantity /*=1*/) {
	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);
	return TryAddItem_Internal(Item);
}

int32 UInventoryComponent::ConsumeItem(UItem* Item) {
	if (Item) {
		ConsumeItem(Item, Item->GetQuantity());
	}
	return 0;
}

int32 UInventoryComponent::ConsumeItem(UItem* Item, const int32 Quantity) {
	if (GetOwner() && GetOwner()->HasAuthority() && Item) {
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

		//We shouldn't have a negative amount of the item after the drop
		ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

		//We now have zero of this item, remove it from the inventory
		Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

		if (Item->GetQuantity() <= 0) {
			RemoveItem(Item);
		} else {
			ClientRefreshInventory();
		}

		return RemoveQuantity;
	}

	return 0;
}

bool UInventoryComponent::RemoveItem(UItem* Item) {
	if (GetOwner() && GetOwner()->HasAuthority()) {
		if (Item) {
			Items.RemoveSingle(Item);
			OnItemRemoved.Broadcast(Item);

			OnRep_Items();

			ReplicatedItemsKey++;

			return true;
		}
	}

	return false;
}

bool UInventoryComponent::HasItem(TSubclassOf <UItem> ItemClass, const int32 Quantity /*= 1*/) const {
	if (UItem* ItemToFind = FindItemByClass(ItemClass)) {
		return ItemToFind->GetQuantity() >= Quantity;
	}
	return false;
}

UItem* UInventoryComponent::FindItem(UItem* Item) const {
	if (Item) {
		for (auto& InvItem : Items) {
			if (InvItem && InvItem->GetClass() == Item->GetClass()) {
				return InvItem;
			}
		}
	}
	return nullptr;
}

UItem* UInventoryComponent::FindItemByClass(TSubclassOf<UItem> ItemClass) const {
	for (auto& InvItem : Items) {
		if (InvItem && InvItem->GetClass() == ItemClass) {
			return InvItem;
		}
	}
	return nullptr;
}

TArray<UItem*> UInventoryComponent::FindItemsByClass(TSubclassOf<UItem> ItemClass) const {
	TArray<UItem*> ItemsOfClass;

	for (auto& InvItem : Items) {
		if (InvItem && InvItem->GetClass()->IsChildOf(ItemClass)) {
			ItemsOfClass.Add(InvItem);
		}
	}

	return ItemsOfClass;
}

float UInventoryComponent::GetCurrentWeight() const {
	float Weight = 0.f;

	for (auto& Item : Items) {
		if (Item) {
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity) {
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity) {
	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::ClientRefreshInventory_Implementation() {
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Items);
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel *Channel, FOutBunch *Bunch, FReplicationFlags *RepFlags) {
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	//Check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey)) {
		for (auto& Item : Items) {
			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->GetRepKey())) {
				bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}

	return bWroteSomething;
}

UItem* UInventoryComponent::AddItem(UItem* Item, const int32 Quantity) {
	if (GetOwner() && GetOwner()->HasAuthority()) {
		UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
		NewItem->SetWorld(GetWorld());
		NewItem->SetQuantity(Quantity);
		NewItem->SetOwningInventory(this);
		NewItem->AddToInventory(this);
		Items.Add(NewItem);
		NewItem->MarkDirtyForReplication();
		OnItemAdded.Broadcast(NewItem);
		OnRep_Items();

		return NewItem;
	}

	return nullptr;
}

void UInventoryComponent::OnRep_Items() {
	OnInventoryUpdated.Broadcast();

	for (auto& Item : Items) {
		//On the client the world won't be set initially, so it set if not
		if (Item && !Item->GetWorld()) {
			OnItemAdded.Broadcast(Item);
			Item->SetWorld(GetWorld());
		}
	}
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(UItem* Item) {
	if (GetOwner() && GetOwner()->HasAuthority()) {
		if (Item->IsStackable()) {
			//Somehow the items quantity went over the max stack size. This shouldn't ever happen
			ensure(Item->GetQuantity() <= Item->GetMaxStackSize());

			if (UItem* ExistingItem = FindItem(Item)) {
				if (ExistingItem->IsStackFull()) {
					return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("StackFullText", "Couldn't add %s. Tried adding items to a stack that was full."), Item->GetItemDisplayName()));
				} else {
					//Find the maximum amount of the item we could take due to weight
					const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->GetItemWeight());
					const int32 QuantityMaxAddAmount = FMath::Min(ExistingItem->GetMaxStackSize() - ExistingItem->GetQuantity(), Item->GetQuantity());
					const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

					if (AddAmount <= 0) {
						//Already did full stack check, must not have enough weight
						return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("StackWeightFullText", "Couldn't add %s, too much weight."), Item->GetItemDisplayName()));
					} else {
						ExistingItem->SetQuantity(ExistingItem->GetQuantity() + AddAmount);
						return AddAmount >= Item->GetQuantity() ? FItemAddResult::AddedAll(Item->GetQuantity()) : FItemAddResult::AddedSome(Item->GetQuantity(), AddAmount, LOCTEXT("StackAddedSomeFullText", "Couldn't add all of stack to inventory."));
					}
				}
			} else { //we want to add a stackable item that doesn't exist in the inventory
				if (Items.Num() + 1 > GetCapacity()) {
					return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add %s to Inventory. Inventory is full."), Item->GetItemDisplayName()));
				}
				
				const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->GetItemWeight());
				const int32 QuantityMaxAddAmount = FMath::Min(Item->GetMaxStackSize(), Item->GetQuantity());
				const int32 AddAmount = FMath::Min(WeightMaxAddAmount, QuantityMaxAddAmount);

				AddItem(Item, AddAmount);

				return AddAmount >= Item->GetQuantity() ? FItemAddResult::AddedAll(Item->GetQuantity()) : FItemAddResult::AddedSome(Item->GetQuantity(), AddAmount, LOCTEXT("StackAddedSomeFullText", "Couldn't add all of stack to inventory."));
			}
		} else { //item isnt stackable
			if (Items.Num() + 1 > GetCapacity()) {
				return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add %s to Inventory. Inventory is full."), Item->GetItemDisplayName()));
			}

			//Items with a weight of zero dont require a weight check
			if (!FMath::IsNearlyZero(Item->GetItemWeight())) {
				if (GetCurrentWeight() + Item->GetItemWeight() > GetWeightCapacity()) {
					return FItemAddResult::AddedNone(Item->GetQuantity(), FText::Format(LOCTEXT("StackWeightFullText", "Couldn't add %s, too much weight."), Item->GetItemDisplayName()));
				}
			}

			//Non-stackables should always have a quantity of 1
			ensure(Item->GetQuantity() == 1);

			AddItem(Item, 1);

			return FItemAddResult::AddedAll(Item->GetQuantity());
		}
	}

	//AddItem should never be called on a client
	return FItemAddResult::AddedNone(-1, LOCTEXT("ErrorMessage", ""));
}

void UInventoryComponent::ItemAdded(UItem* Item) {
	FString RoleString = GetOwner()->HasAuthority() ? "server" : "client";
	UE_LOG(LogTemp, Warning, TEXT("Item added: %s on %s"), *GetNameSafe(Item), *RoleString);
}

void UInventoryComponent::ItemRemoved(UItem* Item) {
	FString RoleString = GetOwner()->HasAuthority() ? "server" : "client";
	UE_LOG(LogTemp, Warning, TEXT("Item Removed: %s on %s"), *GetNameSafe(Item), *RoleString);
}

#undef LOCTEXT_NAME
