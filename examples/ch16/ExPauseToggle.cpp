// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPauseToggle.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPause, Log, All);

void AExPauseToggleController::BeginPlay()
{
	Super::BeginPlay();

	// A world timer runs on game time, which stands still while the game is paused.
	GetWorldTimerManager().SetTimer(ClockTimer, this, &AExPauseToggleController::LogClocks, 1.0f, /*bLoop=*/true);
}

void AExPauseToggleController::ExTogglePause()
{
	const bool bWantPaused = !IsPaused();

	// The game mode decides: SetPause returns false if it refuses.
	const bool bAccepted = SetPause(bWantPaused);

	UE_LOG(LogExPause, Log, TEXT("SetPause(%s) returned %s; IsGamePaused = %s"),
		bWantPaused ? TEXT("true") : TEXT("false"),
		bAccepted ? TEXT("true") : TEXT("false"),
		UGameplayStatics::IsGamePaused(this) ? TEXT("true") : TEXT("false"));
}

void AExPauseToggleController::LogClocks()
{
	++TimerCount;

	// GetTimeSeconds stops while paused; GetRealTimeSeconds never stops.
	const UWorld* World = GetWorld();
	UE_LOG(LogExPause, Log, TEXT("Timer %d: game time %.1f s, real time %.1f s"),
		TimerCount, World->GetTimeSeconds(), World->GetRealTimeSeconds());
}
