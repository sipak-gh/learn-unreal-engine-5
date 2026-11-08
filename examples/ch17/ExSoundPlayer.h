// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSoundPlayer.generated.h"

class USoundBase;

/**
 * Plays a sound in 2D once per second, each time at a slightly different pitch.
 * Drop it in a level, pick any Sound Wave for Sound, and press Play.
 */
UCLASS()
class AExSoundPlayer : public AActor
{
	GENERATED_BODY()

public:
	/** Any sound asset: a Sound Wave, a Sound Cue or a MetaSound Source. */
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	/** How many times BeginPlay's timer plays the sound. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (ClampMin = "0"))
	int32 Repeats = 3;

	/** Plays Sound once at a random pitch between 0.9 and 1.1. */
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayVaried();

protected:
	virtual void BeginPlay() override;

private:
	void HandleTimer();

	FTimerHandle RepeatTimer;
	int32 PlayedCount = 0;
};
