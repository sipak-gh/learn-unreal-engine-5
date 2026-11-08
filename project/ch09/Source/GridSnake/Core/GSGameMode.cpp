// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Snake/GSSnake.h"
#include "TimerManager.h"

AGSGameMode::AGSGameMode()
{
	// C++ defaults. BP_GSGameMode overrides Default Pawn Class with BP_GSSnake,
	// which knows the flipbook assets.
	GameStateClass = AGSGameState::StaticClass();
	PlayerControllerClass = AGSPlayerController::StaticClass();
	DefaultPawnClass = AGSSnake::StaticClass();
}

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s (collect %d gems)"),
		*GetWorld()->GetMapName(), UGSSettings::Get()->GemsPerLevel);

	// A looping one-second timer counts down; the last tick releases the snake.
	CountdownRemaining = CountdownSeconds;
	TickCountdown();
	if (CountdownRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(CountdownTimer, this, &AGSGameMode::TickCountdown, 1.0f, /*bLoop=*/true);
	}
}

void AGSGameMode::TickCountdown()
{
	if (CountdownRemaining > 0)
	{
		UE_LOG(LogGridSnake, Log, TEXT("Countdown: %d"), CountdownRemaining);
		--CountdownRemaining;
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimer);
	UE_LOG(LogGridSnake, Log, TEXT("Go!"));

	if (AGSSnake* Snake = Cast<AGSSnake>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Snake->StartMoving();
	}
}
