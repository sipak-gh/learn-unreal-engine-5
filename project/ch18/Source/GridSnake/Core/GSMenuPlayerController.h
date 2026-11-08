// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSMenuPlayerController.generated.h"

class UGSMainMenuWidget;

/** Shows the main menu and hands the mouse and keyboard to the UI. */
UCLASS()
class AGSMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** W_MainMenu. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSMainMenuWidget> MainMenuWidget;
};
