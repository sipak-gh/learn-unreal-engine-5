// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "UI/GSMenuPanelWidget.h"
#include "GSOptionsWidget.generated.h"

class UCheckBox;

/**
 * W_Options: video options through UGameUserSettings, which saves them to
 * GameUserSettings.ini in the user's Saved folder.
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

private:
	UFUNCTION()
	void HandleFullscreenChanged(bool bIsChecked);

	UFUNCTION()
	void HandleVSyncChanged(bool bIsChecked);
};
