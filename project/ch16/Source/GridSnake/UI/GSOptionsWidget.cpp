// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSOptionsWidget.h"
#include "Components/CheckBox.h"
#include "GameFramework/GameUserSettings.h"

void UGSOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleFullscreenChanged);
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleVSyncChanged);
}

void UGSOptionsWidget::Refresh()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	// Showing the current values: if this fires a change event, it only writes the same value back.
	FullscreenCheckBox->SetIsChecked(Settings->GetFullscreenMode() != EWindowMode::Windowed);
	VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	FullscreenCheckBox->SetFocus();
}

void UGSOptionsWidget::HandleFullscreenChanged(bool bIsChecked)
{
	// Window mode changes have no effect in Play In Editor; try Standalone Game.
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->SetFullscreenMode(bIsChecked ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/false);
}

void UGSOptionsWidget::HandleVSyncChanged(bool bIsChecked)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	Settings->SetVSyncEnabled(bIsChecked);
	Settings->ApplySettings(/*bCheckForCommandLineOverrides=*/false);
}
