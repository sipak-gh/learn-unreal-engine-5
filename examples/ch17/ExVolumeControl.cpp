// Example for chapter 17 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExVolumeControl.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void AExVolumeControl::BeginPlay()
{
	Super::BeginPlay();

	SetClassVolume(Mix, SoundClass, StartVolume);
}

void AExVolumeControl::SetClassVolume(USoundMix* InMix, USoundClass* InClass, float Volume)
{
	if (!InMix || !InClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetClassVolume needs a Sound Mix and a Sound Class"));
		return;
	}

	// 1. Write (or add) the adjuster for this class into the mix.
	//    FadeInTime 0 applies it at once; bApplyToChildren reaches child classes too.
	UGameplayStatics::SetSoundMixClassOverride(this, InMix, InClass, Volume, /*Pitch=*/1.0f,
		/*FadeInTime=*/0.0f, /*bApplyToChildren=*/true);

	// 2. A mix only does something while it is on the stack of active mixes.
	UGameplayStatics::PushSoundMixModifier(this, InMix);

	UE_LOG(LogTemp, Log, TEXT("%s volume = %.2f through %s"), *InClass->GetName(), Volume, *InMix->GetName());
}

void AExVolumeControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A pushed mix stays active until it is popped.
	if (Mix)
	{
		UGameplayStatics::PopSoundMixModifier(this, Mix);
		UE_LOG(LogTemp, Log, TEXT("Popped %s"), *Mix->GetName());
	}

	Super::EndPlay(EndPlayReason);
}
