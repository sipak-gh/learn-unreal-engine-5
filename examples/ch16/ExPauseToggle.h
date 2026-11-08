// Example for chapter 16 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ExPauseToggle.generated.h"

/**
 * A player controller with a console command, ExTogglePause, and a one-second
 * world timer that logs game time and real time. Pause, wait, unpause, and
 * compare the two clocks.
 */
UCLASS()
class AExPauseToggleController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Console: pauses the game if it runs, resumes it if it is paused. */
	UFUNCTION(Exec)
	void ExTogglePause();

protected:
	virtual void BeginPlay() override;

private:
	void LogClocks();

	FTimerHandle ClockTimer;
	int32 TimerCount = 0;
};
