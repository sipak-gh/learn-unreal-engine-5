// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpritePulse.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;

/**
 * Tints a sprite and a flipbook with a colour that pulses between ColorA and
 * ColorB. Tinting needs no extra material: the default sprite materials
 * multiply the texture by the vertex colour that SetSpriteColor sets.
 */
UCLASS()
class AExSpritePulse : public AActor
{
	GENERATED_BODY()

public:
	AExSpritePulse();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	UPROPERTY(EditAnywhere, Category = "Example")
	FLinearColor ColorA = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Example")
	FLinearColor ColorB = FLinearColor(1.0f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Example", meta = (ClampMin = "0"))
	float PulsesPerSecond = 1.0f;

private:
	float RunningTime = 0.0f;
};
