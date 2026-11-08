// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSPauseMenuWidget.h"
#include "Components/Button.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Core/GSPlayerController.h"
#include "Engine/GameInstance.h"

void UGSPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddDynamic(this, &UGSPauseMenuWidget::HandleResumeClicked);
	RestartButton->OnClicked.AddDynamic(this, &UGSPauseMenuWidget::HandleRestartClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UGSPauseMenuWidget::HandleMainMenuClicked);
}

void UGSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResumeButton->SetFocus();
}

void UGSPauseMenuWidget::HandleResumeClicked()
{
	if (AGSPlayerController* PlayerController = GetOwningPlayer<AGSPlayerController>())
	{
		PlayerController->ResumeGame();
	}
}

void UGSPauseMenuWidget::HandleRestartClicked()
{
	// The level stays paused; the subsystem skips its fade while paused.
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->RestartLevel();
}

void UGSPauseMenuWidget::HandleMainMenuClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->ReturnToMainMenu();
}
