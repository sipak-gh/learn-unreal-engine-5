// GridSnake - course project for "Learn Unreal Engine 5".

#include "UI/GSMenuPanelWidget.h"
#include "Components/Button.h"

void UGSMenuPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// NativeOnInitialized runs once per widget object, so bindings made here
	// are never doubled (NativeConstruct runs each time it is added to a parent).
	BackButton->OnClicked.AddDynamic(this, &UGSMenuPanelWidget::HandleBackClicked);
}

void UGSMenuPanelWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}
