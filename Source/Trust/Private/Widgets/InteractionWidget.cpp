// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InteractionWidget.h"

void UInteractionWidget::UpdateInteractionWidget(UInteractionComponent* InteractionComponent) {
	OwningInteractionComponent = InteractionComponent;
	OnUpdateInteractionWidget();
}
