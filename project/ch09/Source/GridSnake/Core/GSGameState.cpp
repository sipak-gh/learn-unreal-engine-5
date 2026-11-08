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
