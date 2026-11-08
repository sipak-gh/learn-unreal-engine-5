// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

/** Where the current level is in its life. */
UENUM(BlueprintType)
enum class EGSLevelPhase : uint8
{
	Countdown,
	Playing,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSGemsChangedSignature, int32, Collected, int32, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSScoreChangedSignature, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSAllGemsCollectedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSPhaseChangedSignature, EGSLevelPhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCountdownChangedSignature, int32, SecondsLeft);

/**
 * The state of the current level that everyone may read: gems, score, phase.
 * The game mode changes it; anyone interested binds to its events instead of
 * checking every frame.
 */
UCLASS()
class AGSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetGemsCollected() const { return GemsCollected; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetGemsRequired() const { return GemsRequired; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	bool AreAllGemsCollected() const { return GemsCollected >= GemsRequired; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetScore() const { return Score; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	EGSLevelPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "GridSnake")
	int32 GetCountdown() const { return Countdown; }

	/** Counts a gem and its points. Fires OnAllGemsCollected when the last one is taken. */
	void AddGem(int32 Points);

	void AddScore(int32 Points);
	void SetPhase(EGSLevelPhase NewPhase);
	void SetCountdown(int32 SecondsLeft);

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSGemsChangedSignature OnGemsChanged;

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSScoreChangedSignature OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSAllGemsCollectedSignature OnAllGemsCollected;

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "GridSnake")
	FGSCountdownChangedSignature OnCountdownChanged;

protected:
	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsCollected = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsRequired = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 Score = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	EGSLevelPhase Phase = EGSLevelPhase::Countdown;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 Countdown = 0;
};
