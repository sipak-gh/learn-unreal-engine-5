// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
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

	// Look this map up in the level list. bRemovePrefixString strips the
	// "UEDPIE_0_" prefix that Play In Editor adds to map names.
	const UGSSettings* Settings = UGSSettings::Get();
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true));
	LevelIndex = LevelList ? LevelList->FindLevelIndex(LevelId) : INDEX_NONE;
	const FGSLevelInfo* LevelInfo = LevelList ? LevelList->GetLevel(LevelIndex) : nullptr;

	PointsPerGem = LevelInfo ? LevelInfo->PointsPerGem : Settings->PointsPerGem;
	UE_LOG(LogGridSnake, Log, TEXT("Level %s: list index %d, %d points per gem"), *LevelId.ToString(), LevelIndex, PointsPerGem);

	// The player's pawn was spawned when the player joined, and has begun play
	// in Super::StartPlay(), so it is safe to configure now.
	if (AGSSnake* Snake = GetSnake())
	{
		Snake->OnDied.AddDynamic(this, &AGSGameMode::HandleSnakeDied);
		Snake->ConfigureSpeed(
			LevelInfo ? LevelInfo->StepInterval : Settings->DefaultStepInterval,
			LevelInfo ? LevelInfo->SpeedCurve.Get() : nullptr);
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

	State->AddGem(PointsPerGem);
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

	GetWorldTimerManager().SetTimer(LevelEndTimer, this, &AGSGameMode::LoadNextLevel, LevelEndDelay, /*bLoop=*/false);
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

void AGSGameMode::LoadNextLevel()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || LevelIndex == INDEX_NONE)
	{
		RestartLevel();
		return;
	}

	int32 NextIndex = LevelIndex + 1;
	if (!LevelList->GetLevel(NextIndex))
	{
		UE_LOG(LogGridSnake, Log, TEXT("That was the last level - starting over"));
		NextIndex = 0;
	}

	const FGSLevelInfo* Next = LevelList->GetLevel(NextIndex);
	UE_LOG(LogGridSnake, Log, TEXT("Opening level %d: %s"), NextIndex, *Next->Map.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Next->Map);
}

AGSGameState* AGSGameMode::GetGSGameState() const
{
	return GetGameState<AGSGameState>();
}

AGSSnake* AGSGameMode::GetSnake() const
{
	return Cast<AGSSnake>(UGameplayStatics::GetPlayerPawn(this, 0));
}
