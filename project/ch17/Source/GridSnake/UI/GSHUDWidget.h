// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GSGameState.h"
#include "GSHUDWidget.generated.h"

class UHorizontalBox;
class UImage;
class UPaperSprite;
class UTextBlock;

/**
 * The in-level HUD. Layout lives in the Widget Blueprint W_HUD (whose parent
 * class is this); logic lives here. Widgets marked BindWidget must exist in
 * W_HUD with exactly the same name, or the blueprint will not compile.
 */
UCLASS(Abstract)
class UGSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** "Gems 1/3". */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	/** Big centred text: the countdown, then level messages. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BannerText;

	/** Optional row of gem icons, one per required gem, filled in from C++. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> GemIconBox;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TObjectPtr<UPaperSprite> GemIconSprite;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FVector2D GemIconSize = FVector2D(32.0, 32.0);

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor CollectedColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor MissingColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.6f);

private:
	// Delegate handlers must be UFUNCTIONs to bind to dynamic delegates.
	UFUNCTION()
	void HandleGemsChanged(int32 Collected, int32 Required);

	UFUNCTION()
	void HandleScoreChanged(int32 Score);

	UFUNCTION()
	void HandlePhaseChanged(EGSLevelPhase Phase);

	UFUNCTION()
	void HandleCountdownChanged(int32 SecondsLeft);

	void BuildGemIcons(int32 Required);
	void SetBanner(const FText& Text);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> GemIcons;

	TWeakObjectPtr<AGSGameState> GameState;
};
