// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "Trust/TrustCharacter.h"
#include "Widgets/InteractionWidget.h"

UInteractionComponent::UInteractionComponent() {
	SetComponentTickEnabled(false);

	InteractionTime = 0.f;
	InteractionDistance = 200.f;
	InteractableNameText = FText::FromString("Interactable Object");
	InteractableActionText = FText::FromString("Interact");
	bAllowMultipleInteractions = true;

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(600, 100);
	bDrawAtDesiredSize = true;

	SetActive(true);
	SetHiddenInGame(true);
}

void UInteractionComponent::SetInteractableNameText(const FText& NewNameText) {
	InteractableNameText = NewNameText;
	RefreshWidget();
}

void UInteractionComponent::SetInteractableActionText(const FText& NewActionText) {
	InteractableActionText = NewActionText;
	RefreshWidget();
}

void UInteractionComponent::Deactivate() {
	Super::Deactivate();

	for (int32 i = Interactors.Num() - 1; i >= 0; --i) {
		if (ATrustCharacter* Interactor = Interactors[i]) {
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}
	
	Interactors.Empty();
}

bool UInteractionComponent::CanInteract(ATrustCharacter* Character) const {
	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractions && Interactors.Num() >= 1;
	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}

void UInteractionComponent::RefreshWidget() {
	if (UInteractionWidget *InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject())) {
		InteractionWidget->UpdateInteractionWidget(this);
	}
}

void UInteractionComponent::BeginFocus(ATrustCharacter* Character) {
	if (!IsActive() || !GetOwner() || !Character) {
		return;
	}

	OnBeginFocus.Broadcast(Character);

	if (GetNetMode() != NM_DedicatedServer) {
		SetHiddenInGame(false);
		
		for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass())) {
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp)) {
				Prim->SetRenderCustomDepth(true);
			}
		}
	}

	RefreshWidget();
}

void UInteractionComponent::EndFocus(ATrustCharacter* Character) {
	OnEndFocus.Broadcast(Character);

	if (GetNetMode() != NM_DedicatedServer) {
		SetHiddenInGame(true);

		for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass())) {
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp)) {
				Prim->SetRenderCustomDepth(false);
			}
		}
	}
}

void UInteractionComponent::BeginInteract(ATrustCharacter* Character) {
	if (CanInteract(Character)) {
		Interactors.AddUnique(Character);
		OnBeginInteract.Broadcast(Character);
	}
}

void UInteractionComponent::EndInteract(ATrustCharacter* Character) {
	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}

void UInteractionComponent::Interact(ATrustCharacter* Character) {
	if (CanInteract(Character)) {
		OnInteract.Broadcast(Character);
	}
}

float UInteractionComponent::GetInteractPercentage() {
	if (Interactors.IsValidIndex(0)) {
		if (ATrustCharacter* Interactor = Interactors[0]) {
			if (Interactor && Interactor->IsInteracting()) {
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractTime() / InteractionTime);
			}
		}
	}
	return 0.f;
}

float UInteractionComponent::GetInteractionDistance() const {
	return InteractionDistance;
}

void UInteractionComponent::SetInteractionDistance(const float NewDistance) {
	InteractionDistance = NewDistance;
}

void UInteractionComponent::SetInteractionTime(const float NewTime) {
	InteractionTime = NewTime;
}
