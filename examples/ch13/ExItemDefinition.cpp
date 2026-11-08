// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExItemDefinition.h"
#include "Engine/Texture2D.h"

FPrimaryAssetId UExItemDefinition::GetPrimaryAssetId() const
{
	// Every item shares one type, so an asset manager query for "ExItem"
	// finds them all, whatever their class or folder.
	static const FName ItemType(TEXT("ExItem"));
	return FPrimaryAssetId(FPrimaryAssetType(ItemType), GetFName());
}

void AExItemInspector::BeginPlay()
{
	Super::BeginPlay();

	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Item in the Details panel"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Item %s: \"%s\", price %d, id %s"),
		*Item->GetName(), *Item->DisplayName.ToString(), Item->Price,
		*Item->GetPrimaryAssetId().ToString());

	if (Item->Icon.IsNull())
	{
		UE_LOG(LogTemp, Log, TEXT("No icon set"));
		return;
	}

	// IsPending: the path is set but the object is not in memory (yet).
	UE_LOG(LogTemp, Log, TEXT("Icon %s, in memory: %s"),
		*Item->Icon.ToString(), Item->Icon.IsPending() ? TEXT("no") : TEXT("yes"));

	// Loads now and blocks until done. Fine for one small texture, not for a level.
	if (UTexture2D* Texture = Item->Icon.LoadSynchronous())
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded %s (%dx%d)"), *Texture->GetName(), Texture->GetSizeX(), Texture->GetSizeY());
	}
}
