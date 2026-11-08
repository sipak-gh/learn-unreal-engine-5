// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExCounterWidget.generated.h"

class UTextBlock;

/**
 * The C++ parent of the Widget Blueprint W_ExCounter. The blueprint owns the
 * layout; this class owns the logic. W_ExCounter must contain a Text widget
 * named exactly CountText, or it will not compile.
 */
UCLASS(Abstract)
class UExCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Stores the value and shows it. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	void SetCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category = "Example")
	int32 GetCount() const { return Count; }

protected:
	virtual void NativeConstruct() override;

	/** Filled in by UMG with the Text widget of the same name in W_ExCounter. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

private:
	int32 Count = 0;
};
