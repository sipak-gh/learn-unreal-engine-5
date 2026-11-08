// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExMusicFader.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Starts Music with a fade-in, sets one MetaSound input, and fades the music out
 * again after HoldSeconds. Logs IsPlaying() along the way.
 */
UCLASS()
class AExMusicFader : public AActor
{
	GENERATED_BODY()

public:
	/** Best with a looping sound: a looping Sound Wave or a MetaSound Source. */
	UPROPERTY(EditAnywhere, Category = "Music")
	TObjectPtr<USoundBase> Music;

	/** Name of a Float input on the MetaSound. Ignored by sounds without that input. */
	UPROPERTY(EditAnywhere, Category = "Music")
	FName ParameterName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, Category = "Music")
	float ParameterValue = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0", Units = "s"))
	float FadeSeconds = 2.0f;

	/** Time from the start of the fade-in to the start of the fade-out. */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0", Units = "s"))
	float HoldSeconds = 5.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartFadeOut();
	void ReportAfterFadeOut();

	/** bAutoDestroy is false, so we own the component; the UPROPERTY keeps it alive. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	FTimerHandle FadeTimer;
};
