// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/GSLevelSelectWidget.h"
#include "UI/GSOptionsWidget.h"

void UGSMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandlePlayClicked);
	LevelSelectButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleLevelSelectClicked);
	OptionsButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleOptionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleQuitClicked);

	// Native delegates bind with AddUObject: the binding is skipped safely if we are destroyed.
	LevelSelectPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);
	OptionsPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);
}

void UGSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ShowMainPanel();
}

void UGSMainMenuWidget::ShowMainPanel()
{
	PanelSwitcher->SetActiveWidget(MainPanel);
	PlayButton->SetFocus();
}

void UGSMainMenuWidget::ShowPanel(UGSMenuPanelWidget* Panel)
{
	PanelSwitcher->SetActiveWidget(Panel);
	Panel->Refresh();
}

void UGSMainMenuWidget::HandlePlayClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(0);
}

void UGSMainMenuWidget::HandleLevelSelectClicked()
{
	ShowPanel(LevelSelectPanel);
}

void UGSMainMenuWidget::HandleOptionsClicked()
{
	ShowPanel(OptionsPanel);
}

void UGSMainMenuWidget::HandleQuitClicked()
{
	// In Play In Editor this ends the session; in a packaged game it closes the window.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}
