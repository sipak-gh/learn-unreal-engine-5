// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/GSGameState.h"
#include "GSPlayerController.generated.h"

class UGSHUDWidget;
class UGSLevelEndWidget;
class UGSPauseMenuWidget;
class UInputAction;
class UInputMappingContext;

/**
 * The player's will. Owns the view (the board's fixed camera, not the pawn),
 * the input mapping context, and the screens: HUD, pause menu, level end.
 */
UCLASS()
class AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

	/** Pauses the world and shows the pause menu. Only while a level is running. */
	void PauseGame();

	/** Hides the pause menu and unpauses. */
	void ResumeGame();

	/** Shows the level-end screen and hands input to it. Called by the game mode. */
	void ShowLevelEnd(EGSLevelResult Result, int32 LevelScore, int32 RunScore);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Keyboard and gamepad bindings for playing a level (IMC_Snake). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** IA_Pause. Its "Trigger when Paused" option must be on, or it could never unpause. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	/** The HUD layout to show while playing (W_HUD). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSHUDWidget> HUDWidgetClass;

	/** W_PauseMenu. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSPauseMenuWidget> PauseMenuWidgetClass;

	/** W_LevelEnd. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSLevelEndWidget> LevelEndWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSHUDWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UGSPauseMenuWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UGSLevelEndWidget> LevelEndWidget;

private:
	void HandlePausePressed();

	/**
	 * Switches to a menu input mode and shows the mouse cursor. Call it before
	 * adding the widget: the widget then moves focus to its first button.
	 */
	void EnterMenuInputMode(bool bGameKeepsInput);
};
