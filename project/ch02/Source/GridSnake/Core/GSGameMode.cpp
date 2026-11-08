// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameMode.h"
#include "GridSnake.h"
#include "Engine/World.h"

AGSGameMode::AGSGameMode()
{
}

void AGSGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogGridSnake, Log, TEXT("GridSnake game mode started on map %s"), *GetWorld()->GetMapName());
}
