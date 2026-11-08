// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExMusicFader.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void AExMusicFader::BeginPlay()
{
	Super::BeginPlay();

	if (!Music)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Music in the Details panel"), *GetName());
		return;
	}

	// Creates the component but does not play it yet.
	MusicComponent = UGameplayStatics::CreateSound2D(this, Music, /*VolumeMultiplier=*/1.0f, /*PitchMultiplier=*/1.0f,
		/*StartTime=*/0.0f, /*ConcurrencySettings=*/nullptr, /*bPersistAcrossLevelTransition=*/false, /*bAutoDestroy=*/false);
	if (!MusicComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSound2D returned null (no audio device?)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Created %s: IsPlaying=%s"), *Music->GetName(), MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	// FadeIn starts playback at volume 0 and ramps up to 1.
	MusicComponent->FadeIn(FadeSeconds);
	UE_LOG(LogTemp, Log, TEXT("After FadeIn(%.1f): IsPlaying=%s"), FadeSeconds, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	// Sets a MetaSound input by name. Safe to call on any sound: unknown names are ignored.
	MusicComponent->SetFloatParameter(ParameterName, ParameterValue);
	UE_LOG(LogTemp, Log, TEXT("Set %s = %.2f"), *ParameterName.ToString(), ParameterValue);

	GetWorldTimerManager().SetTimer(FadeTimer, this, &AExMusicFader::StartFadeOut, HoldSeconds, /*InbLoop=*/false);
}

void AExMusicFader::StartFadeOut()
{
	// Fading to volume 0 stops the sound when the fade ends. Until then it still plays.
	MusicComponent->FadeOut(FadeSeconds, /*FadeVolumeLevel=*/0.0f);
	UE_LOG(LogTemp, Log, TEXT("After FadeOut(%.1f, 0): IsPlaying=%s"), FadeSeconds, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));

	GetWorldTimerManager().SetTimer(FadeTimer, this, &AExMusicFader::ReportAfterFadeOut, FadeSeconds + 0.5f, /*InbLoop=*/false);
}

void AExMusicFader::ReportAfterFadeOut()
{
	UE_LOG(LogTemp, Log, TEXT("%.1f s after the fade-out started: IsPlaying=%s"), FadeSeconds + 0.5f, MusicComponent->IsPlaying() ? TEXT("true") : TEXT("false"));
}

void AExMusicFader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The component is not attached to this actor, so stop it explicitly.
	if (MusicComponent)
	{
		MusicComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}
