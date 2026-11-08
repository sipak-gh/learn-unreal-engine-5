// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Engine/World.h"
#include "Gameplay/GSGem.h"
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

	// The player's pawn was spawned when the player joined, before StartPlay.
	if (AGSSnake* Snake = GetSnake())
	{
		Snake->OnDied.AddDynamic(this, &AGSGameMode::HandleSnakeDied);
	}

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
	AGSGameState* State = GetGSGameState();

	if (CountdownRemaining > 0)
	{
		UE_LOG(LogGridSnake, Log, TEXT("Countdown: %d"), CountdownRemaining);
		State->SetCountdown(CountdownRemaining);
		--CountdownRemaining;
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimer);
	State->SetCountdown(0);
	State->SetPhase(EGSLevelPhase::Playing);

	if (AGSSnake* Snake = GetSnake())
	{
		Snake->StartMoving();
	}
}

void AGSGameMode::OnGemCollected(AGSGem* Gem, AGSSnake* Snake)
{
	AGSGameState* State = GetGSGameState();
	if (State->GetPhase() != EGSLevelPhase::Playing)
	{
		return;
	}

	State->AddGem(UGSSettings::Get()->PointsPerGem);
	Snake->Grow(GrowthPerGem);
}

void AGSGameMode::OnExitReached(AGSSnake* Snake)
{
	AGSGameState* State = GetGSGameState();
	if (State->GetPhase() != EGSLevelPhase::Playing)
	{
		return;
	}

	Snake->StopMoving();
	State->SetPhase(EGSLevelPhase::Completed);
	UE_LOG(LogGridSnake, Log, TEXT("Level complete with %d points"), State->GetScore());

	// Chapter 13 loads the next level from data; for now, play this one again.
	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::RestartLevel, LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::HandleSnakeDied(AGSSnake* Snake)
{
	GetGSGameState()->SetPhase(EGSLevelPhase::Failed);
	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::RestartLevel, LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::RestartLevel()
{
	// bRemovePrefixString strips the "UEDPIE_0_" prefix that Play In Editor adds.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	UE_LOG(LogGridSnake, Log, TEXT("Restarting %s"), *LevelName);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

AGSGameState* AGSGameMode::GetGSGameState() const
{
	return GetGameState<AGSGameState>();
}

AGSSnake* AGSGameMode::GetSnake() const
{
	return Cast<AGSSnake>(UGameplayStatics::GetPlayerPawn(this, 0));
}
