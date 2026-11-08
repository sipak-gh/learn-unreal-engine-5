// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExSpriteActor.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

/**
 * Shows one sprite with a tint. Pick the sprite and the tint in the Details panel;
 * OnConstruction applies them, so the change is visible in the editor at once.
 */
UCLASS()
class AExSpriteActor : public AActor
{
	GENERATED_BODY()

public:
	AExSpriteActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Sprite")
	TObjectPtr<UPaperSpriteComponent> SpriteComponent;

	UPROPERTY(EditAnywhere, Category = "Sprite")
	TObjectPtr<UPaperSprite> Sprite;

	/** Multiplied with the sprite's pixels. White leaves them unchanged. */
	UPROPERTY(EditAnywhere, Category = "Sprite")
	FLinearColor Tint = FLinearColor::White;
};
