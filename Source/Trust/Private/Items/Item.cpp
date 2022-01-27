// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Item.h"

#include "Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "Item"

UItem::UItem() {
	ItemDisplayName = LOCTEXT("ItemName", "Item");
	UseActionText = LOCTEXT("ItemUseActionText", "Item");
	Weight = 0.01f;
	bStackable = true;
	Quantity = 1;
	MaxStackSize = 2;
	RepKey = 0;
}

bool UItem::ShouldShowInInventory() const {
	return true;
}

void UItem::SetQuantity(const int32 NewQuantity) {
	if (NewQuantity != Quantity) {
		Quantity = FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);
		MarkDirtyForReplication();
	}
}

void UItem::Use(ATrustCharacter* Character) {}

void UItem::AddToInventory(UInventoryComponent* Inventory) {}

bool UItem::IsSupportedForNetworking() const {
	return true;
}

UWorld* UItem::GetWorld() const {
	return World;
}

void UItem::SetWorld(UWorld* NewWorld) {
	World = NewWorld;
}

// REPLICATION
void UItem::MarkDirtyForReplication() {
	++RepKey;

	if (OwningInventory) {
		OwningInventory->MarkDirtyForReplication();
	}
}

void UItem::SetOwningInventory(UInventoryComponent* NewOwningInventory) {
	OwningInventory = NewOwningInventory;
}

void UItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItem, Quantity);
}

void UItem::OnRep_Quantity() {
	OnItemModified.Broadcast();
}

#if WITH_EDITOR
void UItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UItem, Quantity)) {
		Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
	}
}
#endif

#undef LOCTEXT_NAMESPACE