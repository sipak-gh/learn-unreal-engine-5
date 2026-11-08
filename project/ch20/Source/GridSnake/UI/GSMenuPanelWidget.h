// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GSMenuPanelWidget.generated.h"

class UButton;

/** A native (C++-only) delegate: cheaper than a dynamic one, but invisible to Blueprints. */
DECLARE_MULTICAST_DELEGATE(FGSBackRequestedSignature);

/**
 * Base class for a sub-screen of the main menu (level select, options).
 * The main menu shows a panel, calls Refresh, and listens for OnBackRequested.
 */
UCLASS(Abstract)
class UGSMenuPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Brings the panel up to date and moves focus into it. */
	virtual void Refresh() {}

	FGSBackRequestedSignature OnBackRequested;

protected:
	virtual void NativeOnInitialized() override;

	/** Called when Back is clicked, before OnBackRequested. */
	virtual void HandleBack() {}

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void HandleBackClicked();
};
