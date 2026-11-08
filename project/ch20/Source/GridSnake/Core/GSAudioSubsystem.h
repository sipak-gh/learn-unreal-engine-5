// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSAudioSubsystem.generated.h"

class UAudioComponent;

/**
 * Music that keeps playing across level loads, and the volume settings.
 * Sound effects are played by the actors themselves (UGameplayStatics::PlaySound2D);
 * their volume follows the SC_SFX sound class, which this subsystem controls.
 */
UCLASS()
class UGSAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Called by the game modes when a map starts: starts the music once and re-applies volumes. */
	void HandleLevelStarted(UWorld* World);

	/** 0 = calm, 1 = all gems collected. Drives the "Intensity" input of the music MetaSound. */
	void SetMusicIntensity(float Intensity);

	/** Volume setters update the save game in memory; the options screen saves it. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "GridSnake|Audio")
	void SetSfxVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "GridSnake|Audio")
	float GetSfxVolume() const { return SfxVolume; }

	/** Name of the float input on MSS_Music. */
	static const FName IntensityParameter;

private:
	/** Writes the volumes into the SMix_Volumes sound mix and makes sure it is active. */
	void ApplyVolumes();

	/**
	 * Created with bPersistAcrossLevelTransition, so it is not owned by any
	 * world and keeps playing while maps load. The UPROPERTY keeps it alive.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

	float MusicVolume = 0.8f;
	float SfxVolume = 1.0f;
};
