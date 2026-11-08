// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExOrthoCamera.generated.h"

class UCameraComponent;

/**
 * An orthographic camera that takes over the player's view when play starts.
 * Place it at Y = 500 with rotation (0, -90, 0) to look at Paper2D sprites.
 */
UCLASS()
class AExOrthoCamera : public AActor
{
	GENERATED_BODY()

public:
	AExOrthoCamera();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Visible width of the view in world units (1 unit = 1 pixel in this course). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "1"))
	float OrthoWidth = 512.0f;

	/** Seconds to blend from the current view to this camera. */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0"))
	float BlendTime = 0.5f;

	/** Changes the visible width. Smaller values zoom in. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ZoomTo(float NewWidth);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;
};
