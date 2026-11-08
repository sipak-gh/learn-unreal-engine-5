// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GSFlipbookEffect.generated.h"

class UPaperFlipbookComponent;

/**
 * Plays its flipbook once, then destroys itself. Make a data-only Blueprint
 * per effect (BP_GSGemPop) and spawn it with SpawnActor.
 */
UCLASS()
class AGSFlipbookEffect : public AActor
{
	GENERATED_BODY()

public:
	AGSFlipbookEffect();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Flipbook;

private:
	UFUNCTION()
	void HandleFinishedPlaying();
};
