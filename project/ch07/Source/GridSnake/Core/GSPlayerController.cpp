// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSPlayerController.h"
#include "GridSnake.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"

AGSPlayerController::AGSPlayerController()
{
	// By default, possessing a pawn makes it the view target. The board's
	// camera should stay in charge, so we manage the view target ourselves.
	bAutoManageActiveCameraTarget = false;
}

void AGSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (AActor* Board = UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()))
	{
		SetViewTarget(Board);
		UE_LOG(LogGridSnake, Log, TEXT("View target: %s"), *Board->GetName());
	}
}
