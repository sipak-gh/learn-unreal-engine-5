// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSPlayerController.h"
#include "GridSnake.h"
#include "Core/GSCheatManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GSHUDWidget.h"
#include "UI/GSLevelEndWidget.h"
#include "UI/GSPauseMenuWidget.h"

AGSPlayerController::AGSPlayerController()
{
	// By default, possessing a pawn makes it the view target. The board's
	// camera should stay in charge, so we manage the view target ourselves.
	bAutoManageActiveCameraTarget = false;

	// Created automatically for a single-player game in non-Shipping builds;
	// its Exec functions become console commands.
	CheatClass = UGSCheatManager::StaticClass();
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// Mapping contexts belong to the local player, so they survive pawn changes.
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
		}
		else
		{
			UE_LOG(LogGridSnake, Warning, TEXT("%s has no DefaultMappingContext - set it in BP_GSPlayerController"), *GetName());
		}
	}

	if (AActor* Board = UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()))
	{
		SetViewTarget(Board);
		UE_LOG(LogGridSnake, Log, TEXT("View target: %s"), *Board->GetName());
	}

	// Coming from a menu, the UI may still own the keyboard: give it back to the game.
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	// Widgets are owned by a player: CreateWidget with the controller as owner.
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UGSHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
}

void AGSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// The pause binding lives on the controller, not the snake: pausing is about
	// the player, not the pawn, and must work even when the snake is dead.
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AGSPlayerController::HandlePausePressed);
		}
	}
}

void AGSPlayerController::HandlePausePressed()
{
	if (IsPaused())
	{
		ResumeGame();
	}
	else
	{
		PauseGame();
	}
}

void AGSPlayerController::PauseGame()
{
	// Only a level in progress can be paused, not one that has just ended.
	const AGSGameState* State = GetWorld()->GetGameState<AGSGameState>();
	const bool bLevelRunning = State && (State->GetPhase() == EGSLevelPhase::Countdown || State->GetPhase() == EGSLevelPhase::Playing);
	if (!bLevelRunning || !PauseMenuWidgetClass || IsPaused())
	{
		return;
	}

	if (!SetPause(true))
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UGSPauseMenuWidget>(this, PauseMenuWidgetClass);
	}
	EnterMenuInputMode(/*bGameKeepsInput=*/true);
	// Z-order 10 draws the menu above the HUD (0).
	PauseMenuWidget->AddToViewport(10);
}

void AGSPlayerController::ResumeGame()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AGSPlayerController::ShowLevelEnd(EGSLevelResult Result, int32 LevelScore, int32 RunScore)
{
	if (!LevelEndWidgetClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no LevelEndWidgetClass"), *GetName());
		return;
	}

	EnterMenuInputMode(/*bGameKeepsInput=*/false);
	LevelEndWidget = CreateWidget<UGSLevelEndWidget>(this, LevelEndWidgetClass);
	LevelEndWidget->AddToViewport(10);
	LevelEndWidget->Setup(Result, LevelScore, RunScore);
}

void AGSPlayerController::EnterMenuInputMode(bool bGameKeepsInput)
{
	if (bGameKeepsInput)
	{
		// Game and UI: widgets get the mouse and focus, but keys they do not use
		// (Escape) still reach the player's input, so the pause key can unpause.
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		// UI only: the game sees no input at all.
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	bShowMouseCursor = true;
}
