// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSHUDWidget.h"
#include "GridSnake.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/GSLevelFlowSubsystem.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"

#define LOCTEXT_NAMESPACE "GridSnakeHUD"

void UGSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AGSGameState* State = GetWorld()->GetGameState<AGSGameState>();
	if (!State)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("HUD: no AGSGameState in this world"));
		return;
	}
	GameState = State;

	State->OnGemsChanged.AddDynamic(this, &UGSHUDWidget::HandleGemsChanged);
	State->OnScoreChanged.AddDynamic(this, &UGSHUDWidget::HandleScoreChanged);
	State->OnPhaseChanged.AddDynamic(this, &UGSHUDWidget::HandlePhaseChanged);
	State->OnCountdownChanged.AddDynamic(this, &UGSHUDWidget::HandleCountdownChanged);

	// Show the level name from the level list, or the map name for unlisted maps.
	const UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	const FGSLevelInfo* Level = LevelFlow ? LevelFlow->GetCurrentLevelInfo() : nullptr;
	LevelText->SetText(Level ? Level->DisplayName : FText::FromString(UGameplayStatics::GetCurrentLevelName(this, true)));

	// Events only report changes, so draw the current values once now.
	BuildGemIcons(State->GetGemsRequired());
	HandleGemsChanged(State->GetGemsCollected(), State->GetGemsRequired());
	HandleScoreChanged(State->GetScore());
	HandleCountdownChanged(State->GetCountdown());
}

void UGSHUDWidget::NativeDestruct()
{
	// The game state may outlive this widget; don't leave it calling a dead object.
	if (AGSGameState* State = GameState.Get())
	{
		State->OnGemsChanged.RemoveAll(this);
		State->OnScoreChanged.RemoveAll(this);
		State->OnPhaseChanged.RemoveAll(this);
		State->OnCountdownChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UGSHUDWidget::HandleGemsChanged(int32 Collected, int32 Required)
{
	GemsText->SetText(FText::Format(LOCTEXT("Gems", "Gems {0}/{1}"), Collected, Required));

	for (int32 Index = 0; Index < GemIcons.Num(); ++Index)
	{
		GemIcons[Index]->SetColorAndOpacity(Index < Collected ? CollectedColor : MissingColor);
	}
}

void UGSHUDWidget::HandleScoreChanged(int32 Score)
{
	// The run score banks earlier levels; Score is this level's points.
	const UGSLevelFlowSubsystem* LevelFlow = GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>();
	const int32 Total = Score + (LevelFlow ? LevelFlow->GetRunScore() : 0);
	ScoreText->SetText(FText::Format(LOCTEXT("Score", "Score {0}"), Total));
}

void UGSHUDWidget::HandlePhaseChanged(EGSLevelPhase Phase)
{
	switch (Phase)
	{
	case EGSLevelPhase::Playing:
		SetBanner(FText::GetEmpty());
		break;
	case EGSLevelPhase::Completed:
		SetBanner(LOCTEXT("Complete", "Level complete!"));
		break;
	case EGSLevelPhase::Failed:
		SetBanner(LOCTEXT("Failed", "Ouch!"));
		break;
	default:
		break;
	}
}

void UGSHUDWidget::HandleCountdownChanged(int32 SecondsLeft)
{
	SetBanner(SecondsLeft > 0 ? FText::AsNumber(SecondsLeft) : FText::GetEmpty());
}

void UGSHUDWidget::BuildGemIcons(int32 Required)
{
	if (!GemIconBox || !GemIconSprite)
	{
		return;
	}

	GemIconBox->ClearChildren();
	GemIcons.Reset();

	for (int32 Index = 0; Index < Required; ++Index)
	{
		// Widgets created at runtime belong to the widget tree, like the ones
		// made in the designer.
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		// A Paper2D sprite is a region of a texture (it implements
		// ISlateTextureAtlasInterface), so a UMG brush can draw it directly.
		Icon->SetBrushResourceObject(GemIconSprite);
		Icon->SetDesiredSizeOverride(GemIconSize);
		Icon->SetColorAndOpacity(MissingColor);

		if (UHorizontalBoxSlot* IconSlot = GemIconBox->AddChildToHorizontalBox(Icon))
		{
			IconSlot->SetPadding(FMargin(2.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		GemIcons.Add(Icon);
	}
}

void UGSHUDWidget::SetBanner(const FText& Text)
{
	BannerText->SetText(Text);
	BannerText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE
