// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG, Slate, SlateCore.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExDynamicList.generated.h"

class UVerticalBox;

/**
 * The parent of W_ExDynamicList, which only needs a Vertical Box named List.
 * The rows are created in C++ at runtime.
 */
UCLASS(Abstract)
class UExDynamicList : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Throws away the old rows and builds Count new ones. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void Rebuild(int32 Count);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> List;

	UPROPERTY(EditAnywhere, Category = "Example", meta = (ClampMin = "0", ClampMax = "50"))
	int32 InitialCount = 5;
};
