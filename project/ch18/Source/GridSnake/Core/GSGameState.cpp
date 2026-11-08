// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameState.h"
#include "GridSnake.h"
#include "Core/GSSettings.h"

void AGSGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents runs when the game state is spawned, before any
	// actor's BeginPlay, so everyone who reads GemsRequired sees the real value.

	GemsRequired = UGSSettings::Get()->GemsPerLevel;
	UE_LOG(LogGridSnake, Log, TEXT("GameState ready: 0/%d gems"), GemsRequired);
}

void AGSGameState::AddGem(int32 Points)
{
	if (AreAllGemsCollected())
	{
		return;
	}

	++GemsCollected;
	UE_LOG(LogGridSnake, Log, TEXT("Gem %d/%d"), GemsCollected, GemsRequired);
	OnGemsChanged.Broadcast(GemsCollected, GemsRequired);
	AddScore(Points);

	if (AreAllGemsCollected())
	{
		UE_LOG(LogGridSnake, Log, TEXT("All gems collected - the exit opens"));
		OnAllGemsCollected.Broadcast();
	}
}

void AGSGameState::AddScore(int32 Points)
{
	Score += Points;
	OnScoreChanged.Broadcast(Score);
}

void AGSGameState::SetPhase(EGSLevelPhase NewPhase)
{
	if (Phase == NewPhase)
	{
		return;
	}

	Phase = NewPhase;
	UE_LOG(LogGridSnake, Log, TEXT("Phase: %s"), *UEnum::GetValueAsString(Phase));
	OnPhaseChanged.Broadcast(Phase);
}

void AGSGameState::SetCountdown(int32 SecondsLeft)
{
	Countdown = SecondsLeft;
	OnCountdownChanged.Broadcast(Countdown);
}
