// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExOneShotEffect.generated.h"

class UPaperFlipbookComponent;

/**
 * Plays its flipbook once and destroys itself. Make a Blueprint child per
 * effect, set its Source Flipbook, and spawn it with SpawnActor.
 */
UCLASS()
class AExOneShotEffect : public AActor
{
	GENERATED_BODY()

public:
	AExOneShotEffect();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

private:
	UFUNCTION()
	void HandleFinished();

	double StartTime = 0.0;
};
