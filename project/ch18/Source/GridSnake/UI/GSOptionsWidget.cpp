// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSOptionsWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Core/GSAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Save/GSSaveSubsystem.h"

void UGSOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleFullscreenChanged);
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UGSOptionsWidget::HandleVSyncChanged);
	MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UGSOptionsWidget::HandleMusicVolumeChanged);
	SfxVolumeSlider->OnValueChanged.AddDynamic(this, &UGSOptionsWidget::HandleSfxVolumeChanged);
	ResetProgressButton->OnClicked.AddDynamic(this, &UGSOptionsWidget::HandleResetProgressClicked);
}

void UGSOptionsWidget::Refresh()
{
	const UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	// Showing the current values: if this fires a change event, it only writes the same value back.
	FullscreenCheckBox->SetIsChecked(Settings->GetFullscreenMode() != EWindowMode::Windowed);
	VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());

	const UGSAudioSubsystem* Audio = GetGameInstance()->GetSubsystem<UGSAudioSubsystem>();
	MusicVolumeSlider->SetValue(Audio->GetMusicVolume());
	SfxVolumeSlider->SetValue(Audio->GetSfxVolume());
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

void UGSOptionsWidget::HandleMusicVolumeChanged(float Value)
{
	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetMusicVolume(Value);
}

void UGSOptionsWidget::HandleSfxVolumeChanged(float Value)
{
	GetGameInstance()->GetSubsystem<UGSAudioSubsystem>()->SetSfxVolume(Value);
}

void UGSOptionsWidget::HandleResetProgressClicked()
{
	GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->ResetProgress();
}

void UGSOptionsWidget::HandleBack()
{
	GetGameInstance()->GetSubsystem<UGSSaveSubsystem>()->Save();
}
