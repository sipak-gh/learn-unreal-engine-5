// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSLevelSelectWidget.h"
#include "GridSnake.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "Save/GSSaveSubsystem.h"
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

	const UGSSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();

	UGSLevelButtonWidget* FirstButton = nullptr;
	for (int32 Index = 0; Index < LevelList->Num(); ++Index)
	{
		const FGSLevelInfo& Level = *LevelList->GetLevel(Index);
		const bool bUnlocked = SaveSubsystem->IsLevelUnlocked(Index);

		// A widget created with another widget as its owner shares its owning player.
		UGSLevelButtonWidget* LevelButton = CreateWidget<UGSLevelButtonWidget>(this, LevelButtonClass);
		LevelButton->Setup(Index, Level, bUnlocked, SaveSubsystem->GetBestScore(Level.GetLevelId()));
		LevelButtonPanel->AddChild(LevelButton);

		if (!FirstButton && bUnlocked)
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
