// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExFlipbookStates.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;

UENUM(BlueprintType)
enum class EExAnimState : uint8
{
	Idle,
	Walk,
	Attack
};

/**
 * A state machine in a few lines: one flipbook per state, a 1 s timer that
 * walks Idle -> Walk -> Attack, and Attack (played once) returning to Idle
 * when its flipbook finishes.
 */
UCLASS()
class AExFlipbookStates : public AActor
{
	GENERATED_BODY()

public:
	AExFlipbookStates();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPaperFlipbookComponent> Sprite;

	/** One flipbook per state. A UENUM is a valid TMap key for a UPROPERTY. */
	UPROPERTY(EditAnywhere, Category = "Example")
	TMap<EExAnimState, TObjectPtr<UPaperFlipbook>> Flipbooks;

private:
	void Advance();
	void EnterState(EExAnimState NewState);
	void PlayFlipbookFor(EExAnimState ForState);

	/** Bound to OnFinishedPlaying, a dynamic delegate: must be a UFUNCTION. */
	UFUNCTION()
	void HandleFinished();

	EExAnimState State = EExAnimState::Idle;
	FTimerHandle Timer;
};
