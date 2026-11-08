// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSoundPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void AExSoundPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Sound in the Details panel"), *GetName());
		return;
	}

	GetWorldTimerManager().SetTimer(RepeatTimer, this, &AExSoundPlayer::HandleTimer, /*InRate=*/1.0f, /*InbLoop=*/true);
}

void AExSoundPlayer::HandleTimer()
{
	PlayVaried();

	if (++PlayedCount >= Repeats)
	{
		GetWorldTimerManager().ClearTimer(RepeatTimer);
	}
}

void AExSoundPlayer::PlayVaried()
{
	if (!Sound)
	{
		return;
	}

	// A small random pitch change stops a repeated sound from sounding mechanical.
	const float Pitch = FMath::FRandRange(0.9f, 1.1f);
	UGameplayStatics::PlaySound2D(this, Sound, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/Pitch);

	UE_LOG(LogTemp, Log, TEXT("Played %s at pitch %.2f"), *Sound->GetName(), Pitch);
}
