// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GSGameState.h"
#include "GSLevelEndWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * W_LevelEnd: the screen after a level ends. One widget, three moods:
 * level complete (Next), crashed (Retry), and run complete (Play Again).
 */
UCLASS(Abstract)
class UGSLevelEndWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(EGSLevelResult InResult, int32 LevelScore, int32 RunScore);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;

	/** "Next Level", or "Play Again" after the last level. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NextButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

private:
	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	EGSLevelResult Result = EGSLevelResult::Completed;
};
