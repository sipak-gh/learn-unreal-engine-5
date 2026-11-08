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

void UGSLevelButtonWidget::Setup(int32 InLevelIndex, const FGSLevelInfo& Level, bool bUnlocked, int32 BestScore)
{
	LevelIndex = InLevelIndex;
	NameText->SetText(FText::Format(LOCTEXT("LevelButton", "{0}. {1}"), LevelIndex + 1, Level.DisplayName));

	// A disabled button cannot be clicked or focused.
	Button->SetIsEnabled(bUnlocked);

	if (DetailText)
	{
		DetailText->SetText(bUnlocked
			? FText::Format(LOCTEXT("Best", "Best {0}"), BestScore)
			: LOCTEXT("Locked", "Locked"));
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
