// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSLevelSelectWidget.h"
#include "GridSnake.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "UI/GSLevelButtonWidget.h"

void UGSLevelSelectWidget::Refresh()
{
	LevelButtonPanel->ClearChildren();

	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || !LevelButtonClass)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("Level select needs a level list and a LevelButtonClass"));
		BackButton->SetFocus();
		return;
	}

	UGSLevelButtonWidget* FirstButton = nullptr;
	for (int32 Index = 0; Index < LevelList->Num(); ++Index)
	{
		// A widget created with another widget as its owner shares its owning player.
		UGSLevelButtonWidget* LevelButton = CreateWidget<UGSLevelButtonWidget>(this, LevelButtonClass);
		LevelButton->Setup(Index, *LevelList->GetLevel(Index));
		LevelButtonPanel->AddChild(LevelButton);

		if (!FirstButton)
		{
			FirstButton = LevelButton;
		}
	}

	if (FirstButton)
	{
		FirstButton->FocusButton();
	}
	else
	{
		BackButton->SetFocus();
	}
}
