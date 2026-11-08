// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFlipbookInfo.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

/** Plays a flipbook and logs its frame count, frame rate and length on BeginPlay. */
UCLASS()
class AExFlipbookInfo : public AActor
{
	GENERATED_BODY()

public:
	AExFlipbookInfo();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Flipbook")
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TObjectPtr<UPaperFlipbook> Flipbook;

	/** 1 = the flipbook's own speed, 2 = twice as fast. */
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0"))
	float PlayRate = 1.0f;
};
