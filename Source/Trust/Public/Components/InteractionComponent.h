#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract, class ATrustCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndInteract, class ATrustCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginFocus, class ATrustCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndFocus, class ATrustCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, class ATrustCharacter*, Character);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TRUST_API UInteractionComponent : public UWidgetComponent {
	GENERATED_BODY()

public:

    UInteractionComponent();

    // DELEGATES
    UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
    FOnBeginInteract OnBeginInteract;

    UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
    FOnEndInteract OnEndInteract;

    UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
    FOnBeginFocus OnBeginFocus;

    UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
    FOnEndFocus OnEndFocus;

    UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
    FOnInteract OnInteract;
    
protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    float InteractionTime;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    float InteractionDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    FText InteractableNameText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    FText InteractableActionText;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
    bool bAllowMultipleInteractions;
    
    UPROPERTY()
    TArray<ATrustCharacter*> Interactors;

public:
    void RefreshWidget();
    
    void BeginFocus(class ATrustCharacter *Character);
    void EndFocus(class ATrustCharacter *Character);

    void BeginInteract(class ATrustCharacter *Character);
    void EndInteract(class ATrustCharacter *Character);

    void Interact(class ATrustCharacter *Character);

    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetInteractPercentage();

    UFUNCTION(BlueprintPure, Category = "Interaction")
    float GetInteractionDistance() const;

    void SetInteractionDistance(const float NewDistance);

    UFUNCTION(BlueprintPure, Category = "Interaction")
    FORCEINLINE float GetInteractionTime() const { return InteractionTime; }

    void SetInteractionTime(const float NewTime);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetInteractableNameText(const FText &NewNameText);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetInteractableActionText(const FText &NewActionText);

protected:
    virtual void Deactivate() override;

    bool CanInteract(class ATrustCharacter *Character) const;
};
