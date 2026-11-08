// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSLevelButtonWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "GridSnakeMenu"

void UGSLevelButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button->OnClicked.AddDynamic(this, &UGSLevelButtonWidget::HandleClicked);
}

void UGSLevelButtonWidget::Setup(int32 InLevelIndex, const FGSLevelInfo& Level)
{
	LevelIndex = InLevelIndex;
	NameText->SetText(FText::Format(LOCTEXT("LevelButton", "{0}. {1}"), LevelIndex + 1, Level.DisplayName));

	if (DetailText)
	{
		DetailText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGSLevelButtonWidget::FocusButton()
{
	Button->SetFocus();
}

void UGSLevelButtonWidget::HandleClicked()
{
	GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>()->StartRun(LevelIndex);
}

#undef LOCTEXT_NAMESPACE
