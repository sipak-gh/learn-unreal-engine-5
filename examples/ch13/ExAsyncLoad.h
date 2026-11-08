// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "ExAsyncLoad.generated.h"

class UTexture2D;

/**
 * Loads a texture in the background instead of blocking the game thread.
 * Place one in a level, pick any texture for SoftTexture, press Play.
 */
UCLASS()
class AExAsyncLoad : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Loading")
	TSoftObjectPtr<UTexture2D> SoftTexture;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnLoaded();

	/** The request. While this handle is alive, the streamable manager keeps the texture loaded. */
	TSharedPtr<FStreamableHandle> Handle;

	/** Once loaded, a UPROPERTY keeps the texture alive for as long as this actor is. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadedTexture;

	uint64 RequestFrame = 0;
};
