// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpinner.generated.h"

class UPaperSpriteComponent;

/**
 * A sprite that spins in the screen plane. Paper2D sprites lie in the X-Z plane,
 * so "spinning on screen" means rotating about the Y axis: that is Pitch.
 */
UCLASS()
class AExSpinner : public AActor
{
	GENERATED_BODY()

public:
	AExSpinner();

	virtual void Tick(float DeltaSeconds) override;

	/** Positive values spin one way, negative values the other. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spinner")
	float DegreesPerSecond = 90.0f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Pick any sprite for it in the Details panel (Source Sprite). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPaperSpriteComponent> Sprite;
};
