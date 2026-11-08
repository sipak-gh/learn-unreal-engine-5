// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExWidgetSpawner.generated.h"

class UExCounterWidget;

/**
 * Shows a counter widget for the first player when play starts and adds one
 * to it every second. Place it in any level and set Widget Class to W_ExCounter.
 */
UCLASS()
class AExWidgetSpawner : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** A Widget Blueprint whose parent class is ExCounterWidget. */
	UPROPERTY(EditAnywhere, Category = "Example")
	TSubclassOf<UExCounterWidget> WidgetClass;

private:
	void AddOne();

	/** UPROPERTY keeps the widget alive while we point at it. */
	UPROPERTY(Transient)
	TObjectPtr<UExCounterWidget> Widget;

	FTimerHandle CountTimer;
};
