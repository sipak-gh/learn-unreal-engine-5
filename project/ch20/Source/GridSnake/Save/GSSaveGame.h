// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GSSaveGame.generated.h"

/**
 * Everything GridSnake remembers between sessions. A USaveGame is serialized
 * property by property, by name: every UPROPERTY that differs from its class
 * default ends up in the .sav file. Adding a field is safe; bump CurrentVersion
 * when the meaning of existing data changes.
 */
UCLASS()
class UGSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	/**
	 * The version this save was written with; the save subsystem sets it.
	 * The default is 0, not CurrentVersion: a value equal to its default is not
	 * written, so an old file would otherwise load claiming to be current.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 SaveVersion = 0;

	/** Levels 0..UnlockedLevelCount-1 can be played. */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	int32 UnlockedLevelCount = 1;

	/** Best level score per level id (the map's asset name, e.g. L_Level01). */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	TMap<FName, int32> BestScores;

	/** Best total for a whole run from the first level to the last. */
	UPROPERTY(VisibleAnywhere, Category = "Progress")
	int32 BestRunScore = 0;

	UPROPERTY(VisibleAnywhere, Category = "Options")
	float MusicVolume = 0.8f;

	UPROPERTY(VisibleAnywhere, Category = "Options")
	float SfxVolume = 1.0f;
};
