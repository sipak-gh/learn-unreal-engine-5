// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSLevelFlowSubsystem.generated.h"

struct FGSLevelInfo;

/**
 * Moves the player through the level list and remembers the run while maps
 * come and go. A game instance subsystem is created with the game instance
 * and lives until the game quits, so its members survive OpenLevel.
 *
 * Get it anywhere with GetGameInstance()->GetSubsystem<UGSLevelFlowSubsystem>().
 */
UCLASS()
class UGSLevelFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Starts a new run at a level: resets the run score and loads the map. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void StartRun(int32 LevelIndex);

	/**
	 * Banks a finished level's score into the run and saves progress.
	 * Returns true if another level follows.
	 */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	bool CompleteLevel(int32 LevelScore);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	bool HasNextLevel() const;

	/** Loads the level after the current one. Does nothing after the last level. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void OpenNextLevel();

	/** Loads the current level again. The run score is kept; the level's own score is lost. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void RestartLevel();

	/** Ends the run and loads the main menu map. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Levels")
	void ReturnToMainMenu();

	/**
	 * Called by the game mode when a level begins. Works out which level this
	 * is (so Play In Editor on any listed map works) and fades the view in.
	 */
	void NotifyLevelStarted(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	/** Points banked from completed levels in this run. */
	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	int32 GetRunScore() const { return RunScore; }

	UFUNCTION(BlueprintPure, Category = "GridSnake|Levels")
	bool IsTransitioning() const { return bTransitioning; }

	/** nullptr when the current map is not in the level list. */
	const FGSLevelInfo* GetCurrentLevelInfo() const;

	/** Seconds for the fade to and from black around a level change. */
	static constexpr float FadeSeconds = 0.4f;

private:
	/** Fades out, then opens the map. The fade hides the last frame of the old level. */
	void OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map);
	void OpenPendingLevel();

	/** Shows a loading screen while the engine loads a map (packaged/standalone games only). */
	void HandlePreLoadMap(const FString& MapName);

	TSoftObjectPtr<UWorld> PendingMap;
	FTimerHandle TransitionTimer;
	int32 CurrentLevelIndex = INDEX_NONE;
	int32 RunScore = 0;
	bool bTransitioning = false;
};
