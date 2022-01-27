// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

class ATrustCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UENUM(BlueprintType)
enum class EItemRarity : uint8 {
	IR_Common UMETA(DisplayName = "Common"),
	IR_Uncommon UMETA(DisplayName = "Uncommon"),
	IR_Rare UMETA(DisplayName = "Rare"),
	IR_Epic UMETA(DisplayName = "Epic"),
	IR_Legendary UMETA(DisplayName = "Legendary")
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class TRUST_API UItem : public UObject {
	GENERATED_BODY()

public:
	UItem();

	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

protected:
	UPROPERTY(Transient)
	UWorld* World;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	UStaticMesh *PickupMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    UTexture2D *Thumbnail;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    FText ItemDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
    FText ItemDescription;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
    FText UseActionText;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    EItemRarity Rarity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
    float Weight;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    bool bStackable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2, EditCondition = bStackable))
    int32 MaxStackSize;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    TSubclassOf<class UItemTooltipWidget> ItemTooltip;

    UPROPERTY()
    class UInventoryComponent* OwningInventory;

    UPROPERTY()
    int32 RepKey;
	
	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bStackable))
	int32 Quantity;

public:
	virtual UWorld* GetWorld() const override;
	
	void SetWorld(UWorld *NewWorld);

	virtual void Use(ATrustCharacter *Character);

	void MarkDirtyForReplication();

	void SetOwningInventory(UInventoryComponent* NewOwningInventory);

	virtual void AddToInventory(UInventoryComponent *Inventory);
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);

	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const;

	UFUNCTION(BlueprintImplementableEvent)
	void OnUse(class ATrustCharacter *Character);

	FORCEINLINE int32 GetRepKey() const { return RepKey; }

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; };

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; };

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE bool IsStackFull() const { return Quantity >= MaxStackSize; }
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE bool IsStackable() const { return bStackable; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetItemWeight() const { return Weight; }
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE int32 GetMaxStackSize() const { return MaxStackSize; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE FText GetItemDisplayName() const { return ItemDisplayName; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE UStaticMesh* GetPickupMesh() const { return PickupMesh; }
	
private:
	UFUNCTION()
    void OnRep_Quantity();
	
    

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const override;
	
    virtual bool IsSupportedForNetworking() const override;
	
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
