// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "UI/GSMenuPanelWidget.h"
#include "GSLevelSelectWidget.generated.h"

class UGSLevelButtonWidget;
class UPanelWidget;

/** W_LevelSelect: one button per entry in the level list, built at runtime. */
UCLASS(Abstract)
class UGSLevelSelectWidget : public UGSMenuPanelWidget
{
	GENERATED_BODY()

public:
	virtual void Refresh() override;

protected:
	/** Any panel works: a Vertical Box, a Wrap Box, a Uniform Grid Panel... */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> LevelButtonPanel;

	/** W_LevelButton. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSLevelButtonWidget> LevelButtonClass;
};
