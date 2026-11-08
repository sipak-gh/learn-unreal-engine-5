// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSMenuGameMode.h"
#include "Core/GSAudioSubsystem.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Core/GSMenuPlayerController.h"
#include "Engine/GameInstance.h"

AGSMenuGameMode::AGSMenuGameMode()
{
	// BP_GSMenuGameMode replaces this with BP_GSMenuPlayerController, which knows W_MainMenu.
	PlayerControllerClass = AGSMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AGSMenuGameMode::StartPlay()
{
	// Clears the "transitioning" flag and fades in, as in every level.
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->NotifyLevelStarted(GetWorld());

	Super::StartPlay();

	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->HandleLevelStarted(GetWorld());
}

bool AGSMenuGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return false;
}
