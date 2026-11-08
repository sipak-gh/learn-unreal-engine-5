// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GSPlayerController.generated.h"

class UGSHUDWidget;
class UInputMappingContext;

/**
 * The player's will. Owns the view (the board's fixed camera, not the pawn)
 * and the input mapping context; later also the screens.
 */
UCLASS()
class AGSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGSPlayerController();

protected:
	virtual void BeginPlay() override;

	/** Keyboard and gamepad bindings for playing a level (IMC_Snake). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** The HUD layout to show while playing (W_HUD). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGSHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGSHUDWidget> HUDWidget;
};
