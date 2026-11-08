// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GSGameState.generated.h"

/**
 * The state of the current level that everyone may read: gems, score.
 * The game mode changes it; the HUD (chapter 15) displays it.
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
	int32 GetScore() const { return Score; }

protected:
	virtual void PostInitializeComponents() override;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsCollected = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 GemsRequired = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "GridSnake")
	int32 Score = 0;
};
