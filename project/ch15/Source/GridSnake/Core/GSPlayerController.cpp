// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSPlayerController.h"
#include "GridSnake.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "UI/GSHUDWidget.h"

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

	// Widgets are owned by a player: CreateWidget with the controller as owner.
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UGSHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
}
