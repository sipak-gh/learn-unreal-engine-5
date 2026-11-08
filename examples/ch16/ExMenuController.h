// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ExMenuController.generated.h"

class UExConfirmDialog;
class UUserWidget;

/**
 * A player controller that shows one menu at a time. Showing a menu hands the
 * keyboard, mouse and gamepad to the UI; hiding it gives them back to the game.
 * Console commands for testing: ExAskConfirm, ExCloseMenu.
 */
UCLASS()
class AExMenuController : public APlayerController
{
	GENERATED_BODY()

public:
	/**
	 * Shows a menu in UI-only input mode with a free mouse cursor.
	 * FocusWidgetName names the widget inside the menu (usually a button) that
	 * gets keyboard and gamepad focus. Leave it None to let the menu choose.
	 */
	UFUNCTION(BlueprintCallable, Category = "Example")
	UUserWidget* ShowMenu(TSubclassOf<UUserWidget> MenuClass, FName FocusWidgetName = NAME_None);

	/** Removes the menu and returns to game-only input. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void HideMenu();

	/** Console: shows ConfirmDialogClass and logs the answer. */
	UFUNCTION(Exec)
	void ExAskConfirm();

	/** Console: same as HideMenu. */
	UFUNCTION(Exec)
	void ExCloseMenu();

protected:
	/** W_ExConfirm. */
	UPROPERTY(EditDefaultsOnly, Category = "Example")
	TSubclassOf<UExConfirmDialog> ConfirmDialogClass;

private:
	void HandleConfirmResult(bool bConfirmed);

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CurrentMenu;
};
