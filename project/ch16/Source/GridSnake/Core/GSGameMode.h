// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/GSGameState.h"
#include "GSGameMode.generated.h"

class AGSGem;
class AGSSnake;
class UGSLevelFlowSubsystem;

/**
 * The rules of a GridSnake level: the countdown, what a gem is worth, what
 * happens when the snake dies or reaches the exit. Only the game mode decides;
 * the game state records; actors report events.
 */
UCLASS()
class AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void StartPlay() override;

	/** Called by a gem the snake has entered. */
	void OnGemCollected(AGSGem* Gem, AGSSnake* Snake);

	/** Called by an open exit the snake has entered. */
	void OnExitReached(AGSSnake* Snake);

protected:
	/** Seconds of "3, 2, 1" before the snake starts moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0"))
	int32 CountdownSeconds = 3;

	/** Segments the snake grows per gem. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0"))
	int32 GrowthPerGem = 2;

	/** Pause between dying (or finishing) and the level-end screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0", Units = "s"))
	float LevelEndDelay = 1.5f;

	/** Shows the level-end screen; the player picks what happens next. */
	void ShowLevelEnd(EGSLevelResult Result);

	UPROPERTY(VisibleInstanceOnly, Category = "Rules")
	int32 PointsPerGem = 100;

	AGSGameState* GetGSGameState() const;
	AGSSnake* GetSnake() const;
	UGSLevelFlowSubsystem* GetLevelFlow() const;

private:
	void TickCountdown();

	UFUNCTION()
	void HandleSnakeDied(AGSSnake* Snake);

	FTimerHandle CountdownTimer;
	FTimerHandle LevelEndTimer;
	int32 CountdownRemaining = 0;
};
