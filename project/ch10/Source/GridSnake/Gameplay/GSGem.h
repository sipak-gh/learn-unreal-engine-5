// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSGem.generated.h"

class UBoxComponent;
class UPaperFlipbookComponent;

/**
 * A collectible gem. Place three in every level. BP_GSGem picks the flipbook;
 * the C++ class builds the components and makes the gem bob.
 */
UCLASS()
class AGSGem : public AActor
{
	GENERATED_BODY()

public:
	AGSGem();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Root. A trigger that overlaps the snake's head box. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

	/** How far the sprite bobs up and down, in world units (1 unit = 1 pixel here). */
	UPROPERTY(EditAnywhere, Category = "Gem", meta = (ClampMin = "0"))
	float BobHeight = 1.5f;

	/** Bobs per second, in radians (2*pi = one full bob). */
	UPROPERTY(EditAnywhere, Category = "Gem", meta = (ClampMin = "0"))
	float BobSpeed = 4.0f;

private:
	/** Random start offset so that several gems do not bob in lockstep. */
	float BobPhase = 0.0f;

	float RunningTime = 0.0f;
};
