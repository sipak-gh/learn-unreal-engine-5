// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSLevelEndWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "GridSnakeMenu"

void UGSLevelEndWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	NextButton->OnClicked.AddDynamic(this, &UGSLevelEndWidget::HandleNextClicked);
	RetryButton->OnClicked.AddDynamic(this, &UGSLevelEndWidget::HandleRetryClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UGSLevelEndWidget::HandleMainMenuClicked);
}

void UGSLevelEndWidget::Setup(EGSLevelResult InResult, int32 LevelScore, int32 RunScore)
{
	Result = InResult;

	switch (Result)
	{
	case EGSLevelResult::Completed:
		TitleText->SetText(LOCTEXT("Completed", "Level Complete!"));
		NextButtonText->SetText(LOCTEXT("NextLevel", "Next Level"));
		break;
	case EGSLevelResult::Failed:
		TitleText->SetText(LOCTEXT("Failed", "You Crashed!"));
		break;
	case EGSLevelResult::RunComplete:
		TitleText->SetText(LOCTEXT("RunComplete", "You Win!"));
		NextButtonText->SetText(LOCTEXT("PlayAgain", "Play Again"));
		break;
	}

	ScoreText->SetText(FText::Format(LOCTEXT("EndScore", "Level score {0}   Total {1}"), LevelScore, RunScore));

	// A completed level's score is already banked, so only a crash offers Retry.
	const bool bFailed = Result == EGSLevelResult::Failed;
	NextButton->SetVisibility(bFailed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	RetryButton->SetVisibility(bFailed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	(bFailed ? RetryButton : NextButton)->SetFocus();
}

void UGSLevelEndWidget::HandleNextClicked()
{
	UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	if (Result == EGSLevelResult::RunComplete)
	{
		LevelFlow->StartRun(0);
	}
	else
	{
		LevelFlow->OpenNextLevel();
	}
}

void UGSLevelEndWidget::HandleRetryClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->RestartLevel();
}

void UGSLevelEndWidget::HandleMainMenuClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->ReturnToMainMenu();
}

#undef LOCTEXT_NAMESPACE
