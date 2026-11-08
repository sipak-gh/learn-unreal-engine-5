// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExPhaseMachine.generated.h"

UENUM(BlueprintType)
enum class EExPhase : uint8
{
	Ready,
	Playing,
	Won,
	Lost
};

/** A four-state phase machine that refuses illegal transitions. */
UCLASS()
class AExPhaseMachine : public AActor
{
	GENERATED_BODY()

public:
	/** Returns false, and reports an ensure, if the transition is not allowed. */
	UFUNCTION(BlueprintCallable, Category = "Example")
	bool SetPhase(EExPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Example")
	EExPhase GetPhase() const { return Phase; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, Category = "Example")
	EExPhase Phase = EExPhase::Ready;

private:
	static bool IsLegal(EExPhase From, EExPhase To);
};
