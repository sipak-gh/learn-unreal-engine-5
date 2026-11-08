// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSMainMenuWidget.generated.h"

class UButton;
class UGSLevelSelectWidget;
class UGSMenuPanelWidget;
class UGSOptionsWidget;
class UTextBlock;
class UWidgetSwitcher;

/**
 * W_MainMenu's parent class. A widget switcher holds three panels: the main
 * buttons, level select and options; only one is visible at a time.
 */
UCLASS(Abstract)
class UGSMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PanelSwitcher;

	/** The first child of PanelSwitcher: the column of main buttons. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> MainPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGSLevelSelectWidget> LevelSelectPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGSOptionsWidget> OptionsPanel;

	/** Starts at the furthest unlocked level. Hidden until there is progress. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LevelSelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** Optional: shows "v" + Project Settings > Project > Description > Project Version. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText;

	void ShowMainPanel();
	void ShowPanel(UGSMenuPanelWidget* Panel);

private:
	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleLevelSelectClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
