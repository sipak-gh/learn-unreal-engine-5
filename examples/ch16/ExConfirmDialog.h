// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, InputCore, UMG, Slate, SlateCore.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExConfirmDialog.generated.h"

class UButton;
class UTextBlock;

/**
 * The parent of W_ExConfirm: a yes/no question. It reports the answer through
 * OnResult and removes itself. Required widgets: Button YesButton, Button
 * NoButton, Text Message.
 */
UCLASS(Abstract)
class UExConfirmDialog : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMessage(const FText& InMessage);

	/**
	 * A native single-cast delegate: true for Yes, false for No.
	 * DECLARE_DELEGATE_OneParam(FMyName, bool) declares this same type with a name.
	 */
	TDelegate<void(bool)> OnResult;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> YesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NoButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Message;

private:
	// Button events are dynamic delegates, so the handlers must be UFUNCTIONs.
	UFUNCTION()
	void HandleYesClicked();

	UFUNCTION()
	void HandleNoClicked();

	void Finish(bool bConfirmed);
};
