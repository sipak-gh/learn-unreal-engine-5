// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExAsyncLoad.h"
#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"

void AExAsyncLoad::BeginPlay()
{
	Super::BeginPlay();

	if (SoftTexture.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: pick a texture for SoftTexture"), *GetName());
		return;
	}

	RequestFrame = GFrameCounter;
	UE_LOG(LogTemp, Log, TEXT("Requested %s on frame %llu"), *SoftTexture.ToString(), RequestFrame);

	// The asset manager owns one streamable manager for the whole game.
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Handle = Streamable.RequestAsyncLoad(SoftTexture.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &AExAsyncLoad::OnLoaded));

	UE_LOG(LogTemp, Log, TEXT("BeginPlay continues without waiting"));
}

void AExAsyncLoad::OnLoaded()
{
	// The soft pointer now resolves: Get() returns the object without loading anything.
	LoadedTexture = SoftTexture.Get();
	UE_LOG(LogTemp, Log, TEXT("Loaded %s on frame %llu (%llu frames later)"),
		*GetNameSafe(LoadedTexture), GFrameCounter, GFrameCounter - RequestFrame);
}

void AExAsyncLoad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Leaving before the load finished: drop the request.
	if (Handle.IsValid() && Handle->IsLoadingInProgress())
	{
		Handle->CancelHandle();
	}
	Handle.Reset();

	Super::EndPlay(EndPlayReason);
}
