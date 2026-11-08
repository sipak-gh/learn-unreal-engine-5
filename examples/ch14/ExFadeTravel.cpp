// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExFadeTravel.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void AExFadeTravel::BeginPlay()
{
	Super::BeginPlay();

	// Every new world starts with a new, unfaded camera manager: fade in from black
	// before the first frame is drawn, so the level never pops onto the screen.
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (bFadeInOnBeginPlay && CameraManager)
	{
		CameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FLinearColor::Black,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/false);
	}

	if (AutoGoSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(Timer, this, &AExFadeTravel::Go, AutoGoSeconds, /*bLoop=*/false);
	}
}

void AExFadeTravel::Go()
{
	if (bGoing)
	{
		return;
	}
	if (NextMap.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: pick a NextMap"), *GetName());
		return;
	}
	bGoing = true;

	// From alpha 0 (clear) to 1 (solid black), and hold it there: the last
	// frame drawn before the load is black, not a frozen game.
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds, FLinearColor::Black,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);
	}
	UE_LOG(LogTemp, Log, TEXT("Fading out over %.2f s"), FadeSeconds);

	// A world timer is fine here: it fires while this world, and this actor,
	// still exist. Travel happens after the timer, never during it.
	GetWorldTimerManager().SetTimer(Timer, this, &AExFadeTravel::Travel, FadeSeconds, /*bLoop=*/false);
}

void AExFadeTravel::Travel()
{
	UE_LOG(LogTemp, Log, TEXT("Opening %s"), *NextMap.ToString());

	// Only requests the travel; the engine loads the map at the start of the next frame.
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, NextMap);
	UE_LOG(LogTemp, Log, TEXT("OpenLevel returned; this actor lives until the next frame"));
}
