// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Save/GSSaveSubsystem.h"
#include "UI/GSLevelSelectWidget.h"
#include "UI/GSOptionsWidget.h"

void UGSMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ContinueButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleContinueClicked);
	PlayButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandlePlayClicked);
	LevelSelectButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleLevelSelectClicked);
	OptionsButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleOptionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UGSMainMenuWidget::HandleQuitClicked);

	// Native delegates bind with AddUObject: the binding is skipped safely if we are destroyed.
	LevelSelectPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);
	OptionsPanel->OnBackRequested.AddUObject(this, &UGSMainMenuWidget::ShowMainPanel);

	if (VersionText)
	{
		// Read straight from the game config (DefaultGame.ini, staged with the packaged game).
		FString Version;
		GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni);
		VersionText->SetText(FText::FromString(FString::Printf(TEXT("v%s"), *Version)));
	}
}

void UGSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ShowMainPanel();
}

void UGSMainMenuWidget::ShowMainPanel()
{
	PanelSwitcher->SetActiveWidget(MainPanel);

	// Progress can change on the options screen (Reset Progress), so check every time.
	const bool bCanContinue = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->HasProgress();
	ContinueButton->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	(bCanContinue ? ContinueButton : PlayButton)->SetFocus();
}

void UGSMainMenuWidget::ShowPanel(UGSMenuPanelWidget* Panel)
{
	PanelSwitcher->SetActiveWidget(Panel);
	Panel->Refresh();
}

void UGSMainMenuWidget::HandleContinueClicked()
{
	const int32 LastUnlocked = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->GetUnlockedLevelCount() - 1;
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(LastUnlocked);
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
