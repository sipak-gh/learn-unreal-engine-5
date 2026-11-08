// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#include "ExMenuController.h"
#include "Blueprint/UserWidget.h"
#include "ExConfirmDialog.h"

DEFINE_LOG_CATEGORY_STATIC(LogExMenu, Log, All);

UUserWidget* AExMenuController::ShowMenu(TSubclassOf<UUserWidget> MenuClass, FName FocusWidgetName)
{
	if (!MenuClass)
	{
		UE_LOG(LogExMenu, Warning, TEXT("ShowMenu: no menu class"));
		return nullptr;
	}

	// One menu at a time.
	if (CurrentMenu)
	{
		CurrentMenu->RemoveFromParent();
	}

	CurrentMenu = CreateWidget<UUserWidget>(this, MenuClass);
	CurrentMenu->AddToViewport(/*ZOrder=*/10);

	// UI only: keys and buttons go to widgets, the game sees nothing.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	// The input mode can move focus too. It is applied after this function, so
	// it wins over any SetFocus the menu did in NativeConstruct.
	UWidget* FocusTarget = FocusWidgetName.IsNone() ? nullptr : CurrentMenu->GetWidgetFromName(FocusWidgetName);
	if (FocusTarget)
	{
		InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
	}

	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	UE_LOG(LogExMenu, Log, TEXT("Menu shown: %s (input: UI only, focus %s)"),
		*CurrentMenu->GetName(), FocusTarget ? *FocusTarget->GetName() : TEXT("left to the menu"));
	return CurrentMenu;
}

void AExMenuController::HideMenu()
{
	if (!CurrentMenu)
	{
		return;
	}

	CurrentMenu->RemoveFromParent();
	CurrentMenu = nullptr;

	// Game only also gives keyboard focus back to the game viewport.
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	UE_LOG(LogExMenu, Log, TEXT("Menu hidden (input: game only)"));
}

void AExMenuController::ExAskConfirm()
{
	UExConfirmDialog* Dialog = Cast<UExConfirmDialog>(ShowMenu(ConfirmDialogClass, TEXT("NoButton")));
	if (!Dialog)
	{
		return;
	}

	Dialog->SetMessage(NSLOCTEXT("ExMenu", "Question", "Is this the right answer?"));
	// Native delegates bind to plain member functions; BindUObject also checks
	// that this controller still exists before calling it.
	Dialog->OnResult.BindUObject(this, &AExMenuController::HandleConfirmResult);
}

void AExMenuController::ExCloseMenu()
{
	HideMenu();
}

void AExMenuController::HandleConfirmResult(bool bConfirmed)
{
	UE_LOG(LogExMenu, Log, TEXT("Controller heard: %s"), bConfirmed ? TEXT("yes") : TEXT("no"));
	HideMenu();
}
