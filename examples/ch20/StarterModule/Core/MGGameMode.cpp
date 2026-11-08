// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "Core/MGGameMode.h"
#include "MyGame.h"
#include "Core/MGFlowSubsystem.h"
#include "Core/MGPlayerController.h"
#include "Engine/GameInstance.h"
#include "Player/MGPawn2D.h"

AMGGameMode::AMGGameMode()
{
	// C++ defaults; the Blueprint child replaces the pawn with one that has its assets set.
	PlayerControllerClass = AMGPlayerController::StaticClass();
	DefaultPawnClass = AMGPawn2D::StaticClass();
}

void AMGGameMode::StartPlay()
{
	// Before Super::StartPlay(), which calls BeginPlay on every actor, so actors can
	// already ask the flow subsystem which level this is.
	GetGameInstance()->GetSubsystem<UMGFlowSubsystem>()->NotifyLevelStarted(GetWorld());

	Super::StartPlay();
}
