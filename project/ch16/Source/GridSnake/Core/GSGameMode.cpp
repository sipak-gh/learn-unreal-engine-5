// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
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
	// The subsystem outlives this game mode: it knows which level of the run
	// this is and fades the view in. Tell it before Super::StartPlay() calls
	// BeginPlay on every actor, so they can already ask which level this is.
	UGSLevelFlowSubsystem* LevelFlow = GetLevelFlow();
	LevelFlow->NotifyLevelStarted(GetWorld());

	Super::StartPlay();

	const UGSSettings* Settings = UGSSettings::Get();
	const FGSLevelInfo* LevelInfo = LevelFlow->GetCurrentLevelInfo();
	PointsPerGem = LevelInfo ? LevelInfo->PointsPerGem : Settings->PointsPerGem;

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
	const bool bHasNextLevel = GetLevelFlow()->CompleteLevel(State->GetScore());

	// CreateUObject can carry extra arguments ("payload") for the bound function.
	const EGSLevelResult Result = bHasNextLevel ? EGSLevelResult::Completed : EGSLevelResult::RunComplete;
	GetWorldTimerManager().SetTimer(LevelEndTimer,
		FTimerDelegate::CreateUObject(this, &AGSGameMode::ShowLevelEnd, Result), LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::HandleSnakeDied(AGSSnake* Snake)
{
	GetGSGameState()->SetPhase(EGSLevelPhase::Failed);
	GetWorldTimerManager().SetTimer(LevelEndTimer,
		FTimerDelegate::CreateUObject(this, &AGSGameMode::ShowLevelEnd, EGSLevelResult::Failed), LevelEndDelay, /*bLoop=*/false);
}

void AGSGameMode::ShowLevelEnd(EGSLevelResult Result)
{
	if (AGSPlayerController* PlayerController = Cast<AGSPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ShowLevelEnd(Result, GetGSGameState()->GetScore(), GetLevelFlow()->GetRunScore());
	}
}

AGSGameState* AGSGameMode::GetGSGameState() const
{
	return GetGameState<AGSGameState>();
}

UGSLevelFlowSubsystem* AGSGameMode::GetLevelFlow() const
{
	return GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
}

AGSSnake* AGSGameMode::GetSnake() const
{
	return Cast<AGSSnake>(UGameplayStatics::GetPlayerPawn(this, 0));
}
