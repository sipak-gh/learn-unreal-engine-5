// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#include "ExCounterWidget.h"
#include "Components/TextBlock.h"

void UExCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Replace the designer's preview text with the real value.
	SetCount(Count);
}

void UExCounterWidget::SetCount(int32 NewCount)
{
	Count = NewCount;

	// AsNumber formats for the current culture: 12345 shows as "12,345" in English.
	CountText->SetText(FText::AsNumber(Count));
}
