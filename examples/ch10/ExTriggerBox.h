// Chapter 10 example: a trigger box that logs what enters and leaves it.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExTriggerBox.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;

/** Place across a moving actor's path and press Play: logs enter and leave. */
UCLASS()
class AExTriggerBox : public AActor
{
	GENERATED_BODY()

public:
	AExTriggerBox();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Box;

private:
	// Dynamic delegates find their handler by name through reflection, so
	// handlers must be UFUNCTIONs with exactly the delegate's parameters.
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
