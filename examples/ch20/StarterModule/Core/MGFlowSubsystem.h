// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MGFlowSubsystem.generated.h"

/**
 * Moves the player between the main menu and the levels listed in UMGSettings, with a
 * fade to black around each change. Lives as long as the game instance, so its members
 * survive OpenLevel. Distilled from GridSnake's UGSLevelFlowSubsystem (chapter 15).
 *
 * Get it anywhere with GetGameInstance()->GetSubsystem<UMGFlowSubsystem>().
 */
UCLASS()
class UMGFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Loads a level from the list. Logs a warning for an invalid index. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Flow")
	void OpenLevelByIndex(int32 LevelIndex);

	/** Loads the level after the current one. Returns false after the last level. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Flow")
	bool OpenNextLevel();

	/** Loads the current map again. */
	UFUNCTION(BlueprintCallable, Category = "MyGame|Flow")
	void RestartLevel();

	UFUNCTION(BlueprintCallable, Category = "MyGame|Flow")
	void ReturnToMainMenu();

	/** Called by AMGGameMode when a map begins: finds its index and fades in. */
	void NotifyLevelStarted(UWorld* World);

	/** INDEX_NONE (-1) on the main menu or on a map that is not in the list. */
	UFUNCTION(BlueprintPure, Category = "MyGame|Flow")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

private:
	void OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map);
	void OpenPendingLevel();

	TSoftObjectPtr<UWorld> PendingMap;
	FTimerHandle TransitionTimer;
	int32 CurrentLevelIndex = INDEX_NONE;
	bool bTransitioning = false;
};
