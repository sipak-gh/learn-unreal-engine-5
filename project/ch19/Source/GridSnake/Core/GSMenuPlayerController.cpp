// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSMenuPlayerController.h"
#include "GridSnake.h"
#include "UI/GSMainMenuWidget.h"

void AGSMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no MainMenuWidgetClass - set it in BP_GSMenuPlayerController"), *GetName());
		return;
	}

	// UI only: game input is ignored and the cursor is free. Set it before
	// adding the menu, which then focuses its Play button so arrow keys and
	// the D-pad can move between buttons.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	MainMenuWidget = CreateWidget<UGSMainMenuWidget>(this, MainMenuWidgetClass);
	MainMenuWidget->AddToViewport();
}
