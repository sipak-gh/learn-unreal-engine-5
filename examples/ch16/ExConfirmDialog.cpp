// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#include "ExConfirmDialog.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogExConfirm, Log, All);

void UExConfirmDialog::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Once per widget object, so the bindings can never be added twice.
	YesButton->OnClicked.AddDynamic(this, &UExConfirmDialog::HandleYesClicked);
	NoButton->OnClicked.AddDynamic(this, &UExConfirmDialog::HandleNoClicked);
}

void UExConfirmDialog::NativeConstruct()
{
	Super::NativeConstruct();

	// The safe answer gets the focus, so a stray Enter or A press does no harm.
	NoButton->SetFocus();
}

void UExConfirmDialog::SetMessage(const FText& InMessage)
{
	Message->SetText(InMessage);
}

FReply UExConfirmDialog::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Keys the focused button does not use bubble up to us. Escape or the
	// gamepad's right face button (B on Xbox) mean "no". In Play In Editor,
	// Escape stops the session before the game sees it.
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Finish(false);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UExConfirmDialog::HandleYesClicked()
{
	Finish(true);
}

void UExConfirmDialog::HandleNoClicked()
{
	Finish(false);
}

void UExConfirmDialog::Finish(bool bConfirmed)
{
	UE_LOG(LogExConfirm, Log, TEXT("Confirm result: %s"), bConfirmed ? TEXT("true") : TEXT("false"));

	// Off the screen first: the listener may open another menu.
	RemoveFromParent();
	OnResult.ExecuteIfBound(bConfirmed);
}
