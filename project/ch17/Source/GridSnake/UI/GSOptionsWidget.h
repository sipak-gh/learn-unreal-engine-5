// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "UI/GSMenuPanelWidget.h"
#include "GSOptionsWidget.generated.h"

class UCheckBox;
class USlider;

/**
 * W_Options: video options through UGameUserSettings, which saves them to
 * GameUserSettings.ini in the user's Saved folder, and volumes through the
 * audio subsystem.
 */
UCLASS(Abstract)
class UGSOptionsWidget : public UGSMenuPanelWidget
{
	GENERATED_BODY()

public:
	virtual void Refresh() override;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> FullscreenCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> VSyncCheckBox;

	/** Sliders run from 0 to 1 (the default range). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SfxVolumeSlider;

private:
	UFUNCTION()
	void HandleFullscreenChanged(bool bIsChecked);

	UFUNCTION()
	void HandleVSyncChanged(bool bIsChecked);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleSfxVolumeChanged(float Value);
};
