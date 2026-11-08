// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSGameMode.generated.h"

/**
 * The rules of a GridSnake level. Only the server (in single player: the game)
 * has a game mode. BP_GSGameMode derives from this class to pick assets.
 */
UCLASS()
class AGSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSGameMode();

	virtual void StartPlay() override;

protected:
	/** Seconds of "3, 2, 1" before the snake starts moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Rules", meta = (ClampMin = "0"))
	int32 CountdownSeconds = 3;

private:
	void TickCountdown();

	FTimerHandle CountdownTimer;
	int32 CountdownRemaining = 0;
};
