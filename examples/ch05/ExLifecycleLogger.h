// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExLifecycleLogger.generated.h"

/**
 * Logs every lifecycle function it overrides, with the object's name and whether
 * it is the class default object (CDO). Drop it in a level and watch the Output Log
 * while you place it, move it, press Play and press Stop.
 */
UCLASS()
class AExLifecycleLogger : public AActor
{
	GENERATED_BODY()

public:
	AExLifecycleLogger();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	/** If above zero, the actor destroys itself this many seconds after BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Lifecycle", meta = (ClampMin = "0"))
	float DestroyAfterSeconds = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void LogStage(const TCHAR* Stage) const;
};
