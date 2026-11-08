// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "ExItemDefinition.generated.h"

class UTexture2D;

/**
 * One kind of item, edited as an asset instead of in code.
 * Create one with Content Browser > right-click > Miscellaneous > Data Asset >
 * ExItemDefinition, for example DA_Sword.
 */
UCLASS(BlueprintType)
class UExItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 Price = 10;

	/** Soft: saving the item stores the texture's path; the texture loads only on request. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** The asset manager's name for this asset: type "ExItem", name = the asset name. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

/** Place one in a level, set Item, press Play: it logs the item and loads its icon. */
UCLASS()
class AExItemInspector : public AActor
{
	GENERATED_BODY()

public:
	/** A hard reference: the item asset loads together with the level. */
	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UExItemDefinition> Item;

protected:
	virtual void BeginPlay() override;
};
