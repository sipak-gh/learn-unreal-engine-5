// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG, Slate, SlateCore.

#include "ExDynamicList.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

DEFINE_LOG_CATEGORY_STATIC(LogExList, Log, All);

#define LOCTEXT_NAMESPACE "ExDynamicList"

void UExDynamicList::NativeConstruct()
{
	Super::NativeConstruct();

	Rebuild(InitialCount);
}

void UExDynamicList::Rebuild(int32 Count)
{
	List->ClearChildren();

	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Widgets made at runtime belong to the widget tree, like designer widgets.
		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::Format(LOCTEXT("Row", "Row {0} of {1}"), Index + 1, Count));

		// Adding a child creates its slot: the layout settings live there.
		// (Don't name the variable "Slot": UWidget already has a member called Slot.)
		if (UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	UE_LOG(LogExList, Log, TEXT("%s now has %d rows"), *GetName(), List->GetChildrenCount());
}

#undef LOCTEXT_NAMESPACE
