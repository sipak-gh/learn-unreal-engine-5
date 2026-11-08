// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GSSaveSubsystem.generated.h"

class UGSSaveGame;

/**
 * Owns the one save game: loads it when the game starts, answers questions
 * about progress, and writes it to disk after changes.
 * Saves go to Saved/SaveGames/GridSnake.sav (per user on most platforms).
 */
UCLASS()
class UGSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	bool IsLevelUnlocked(int32 LevelIndex) const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetUnlockedLevelCount() const;

	/** True once the player has unlocked anything beyond the first level. */
	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	bool HasProgress() const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetBestScore(FName LevelId) const;

	UFUNCTION(BlueprintPure, Category = "GridSnake|Save")
	int32 GetBestRunScore() const;

	void UnlockLevel(int32 LevelIndex);

	/** Records a level score. Returns true if it is a new best for that level. */
	bool SubmitLevelScore(FName LevelId, int32 Score);

	/** Records a finished run's total. Returns true if it is a new best. */
	bool SubmitRunScore(int32 Score);

	float GetMusicVolume() const;
	float GetSfxVolume() const;
	void SetVolumes(float MusicVolume, float SfxVolume);

	/** Forgets unlocked levels and scores; keeps the options. Saves immediately. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Save")
	void ResetProgress();

	/** Writes the save game to disk in the background. */
	UFUNCTION(BlueprintCallable, Category = "GridSnake|Save")
	void Save();

	static const FString SlotName;
	static constexpr int32 UserIndex = 0;

private:
	void HandleSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bSuccess);

	/** Always valid after Initialize: loaded from disk, or a fresh one. */
	UPROPERTY(Transient)
	TObjectPtr<UGSSaveGame> SaveGame;
};
