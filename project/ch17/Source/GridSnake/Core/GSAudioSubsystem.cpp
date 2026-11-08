// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSAudioSubsystem.h"
#include "GridSnake.h"
#include "Components/AudioComponent.h"
#include "Core/GSSettings.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

const FName UGSAudioSubsystem::IntensityParameter(TEXT("Intensity"));

void UGSAudioSubsystem::Deinitialize()
{
	// The music is not owned by a world, so nothing else would stop it
	// when Play In Editor ends.
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	Super::Deinitialize();
}

void UGSAudioSubsystem::HandleLevelStarted(UWorld* World)
{
	ApplyVolumes();
	SetMusicIntensity(0.0f);

	if (MusicComponent && MusicComponent->IsPlaying())
	{
		return;
	}

	USoundBase* Music = UGSSettings::Get()->Music.LoadSynchronous();
	if (!Music)
	{
		return;
	}

	MusicComponent = UGameplayStatics::CreateSound2D(World, Music, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/1.0f,
		/*StartTime=*/0.0f, /*ConcurrencySettings=*/nullptr, /*bPersistAcrossLevelTransition=*/true, /*bAutoDestroy=*/false);
	if (MusicComponent)
	{
		MusicComponent->FadeIn(/*FadeInDuration=*/1.0f);
		UE_LOG(LogGridSnake, Log, TEXT("Music started: %s"), *Music->GetName());
	}
}

void UGSAudioSubsystem::SetMusicIntensity(float Intensity)
{
	if (MusicComponent)
	{
		// Does nothing if the sound has no input with this name (a plain Sound Wave).
		MusicComponent->SetFloatParameter(IntensityParameter, FMath::Clamp(Intensity, 0.0f, 1.0f));
	}
}

void UGSAudioSubsystem::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumes();
}

void UGSAudioSubsystem::SetSfxVolume(float Volume)
{
	SfxVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyVolumes();
}

void UGSAudioSubsystem::ApplyVolumes()
{
	const UGSSettings* Settings = UGSSettings::Get();
	USoundMix* Mix = Settings->VolumeMix.LoadSynchronous();
	USoundClass* MusicClass = Settings->MusicClass.LoadSynchronous();
	USoundClass* SfxClass = Settings->SfxClass.LoadSynchronous();
	if (!Mix || !MusicClass || !SfxClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("Volume settings need VolumeMix, MusicClass and SfxClass in Project Settings > GridSnake"));
		return;
	}

	// A sound mix scales the volume of every sound whose Sound Class it overrides.
	// Overrides only take effect while the mix is pushed (active).
	UGameInstance* GameInstance = GetGameInstance();
	UGameplayStatics::SetSoundMixClassOverride(GameInstance, Mix, MusicClass, MusicVolume, /*Pitch=*/1.0f, /*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);
	UGameplayStatics::SetSoundMixClassOverride(GameInstance, Mix, SfxClass, SfxVolume, /*Pitch=*/1.0f, /*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);
	// Pushing a mix that is already active only adds a reference; we never pop it.
	UGameplayStatics::PushSoundMixModifier(GameInstance, Mix);
}
