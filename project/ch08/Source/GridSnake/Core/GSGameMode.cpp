// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Core/GSGameState.h"
#include "Core/GSPlayerController.h"
#include "Core/GSSettings.h"
#include "Engine/World.h"
#include "Snake/GSSnake.h"

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
}
