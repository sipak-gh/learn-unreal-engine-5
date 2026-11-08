// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSLevelButtonWidget.generated.h"

class UButton;
class UTextBlock;
struct FGSLevelInfo;

/** One entry in level select (W_LevelButton): click it to start a run at that level. */
UCLASS(Abstract)
class UGSLevelButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** A locked level shows a disabled button; an unlocked one shows its best score. */
	void Setup(int32 InLevelIndex, const FGSLevelInfo& Level, bool bUnlocked, int32 BestScore);

	/** Gives keyboard/gamepad focus to the button inside. */
	void FocusButton();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** Optional second line (best score, from chapter 18). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailText;

private:
	UFUNCTION()
	void HandleClicked();

	int32 LevelIndex = INDEX_NONE;
};
