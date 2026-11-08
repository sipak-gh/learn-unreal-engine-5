// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MGPlayerController.generated.h"

class UInputMappingContext;

/**
 * Adds the input mapping context and, optionally, looks through a fixed camera actor
 * instead of the pawn's own camera (the GridSnake board pattern). Make a Blueprint
 * child (BP_MGPlayerController) to set the properties.
 */
UCLASS()
class AMGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	/** The default key bindings (IMC_Default). */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/**
	 * Leave empty to view through the pawn's camera (a follow camera). Set it to a class
	 * placed in every level (a board or a CameraActor) for a fixed, single-screen view.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<AActor> FixedViewTargetClass;
};
