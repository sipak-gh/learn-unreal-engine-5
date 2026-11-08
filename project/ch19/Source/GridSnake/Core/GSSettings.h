// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GSSettings.generated.h"

class UGSLevelList;
class USoundBase;
class USoundClass;
class USoundMix;

/**
 * Project-wide game rules. Appears under Project Settings > Game > GridSnake and
 * is saved to Config/DefaultGame.ini. Read it anywhere with UGSSettings::Get().
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GridSnake"))
class UGSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** How many gems must be collected before a level's exit opens. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "1"))
	int32 GemsPerLevel = 3;

	/** Seconds between two snake steps in maps that are not in the level list. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0.02", Units = "s"))
	float DefaultStepInterval = 0.15f;

	/** Score awarded for each gem in maps that are not in the level list. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0"))
	int32 PointsPerGem = 100;

	/** The map with the main menu, loaded by "Main Menu" buttons. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuMap;

	/** The levels, in play order. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UGSLevelList> LevelList;

	/** Looping music (MSS_Music), started once and kept playing across levels. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundBase> Music;

	/** The mix that applies the player's volume settings (SMix_Volumes). */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> VolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxClass;

	/** The class default object holds the values loaded from the .ini files. */
	static const UGSSettings* Get() { return GetDefault<UGSSettings>(); }

	/** Loads the level list on first use (a small asset, so loading synchronously is fine). */
	static const UGSLevelList* GetLevelList();
};
