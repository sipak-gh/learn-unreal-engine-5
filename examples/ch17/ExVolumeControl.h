// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExVolumeControl.generated.h"

class USoundClass;
class USoundMix;

/**
 * Sets the volume of one Sound Class through a Sound Mix: the same two calls
 * GridSnake's audio subsystem makes for its Music and SFX sliders.
 */
UCLASS()
class AExVolumeControl : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Volume")
	TObjectPtr<USoundMix> Mix;

	UPROPERTY(EditAnywhere, Category = "Volume")
	TObjectPtr<USoundClass> SoundClass;

	/** Applied in BeginPlay. 0 = silent, 1 = unchanged. */
	UPROPERTY(EditAnywhere, Category = "Volume", meta = (ClampMin = "0", ClampMax = "1"))
	float StartVolume = 0.25f;

	/** Scales every sound of InClass (and its child classes) by Volume while InMix is active. */
	UFUNCTION(BlueprintCallable, Category = "Volume")
	void SetClassVolume(USoundMix* InMix, USoundClass* InClass, float Volume);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
