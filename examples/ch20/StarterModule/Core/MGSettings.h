// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MGSettings.generated.h"

/**
 * Project-wide settings. Appears under Project Settings > Game > My Game and is saved
 * to Config/DefaultGame.ini. Read it anywhere with UMGSettings::Get().
 *
 * Soft pointers do not load anything by themselves, and the cooker does not follow
 * config files: list every map here in Project Settings > Packaging > "List of maps to
 * include in a packaged build" as well (chapter 20).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "My Game"))
class UMGSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** The map with the main menu. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuMap;

	/** The playable levels, in order. Index 0 is the first level. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TArray<TSoftObjectPtr<UWorld>> Levels;

	/** Seconds for the fade to and from black around a level change. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", Units = "s"))
	float FadeSeconds = 0.4f;

	/** The class default object holds the values loaded from the .ini files. */
	static const UMGSettings* Get() { return GetDefault<UMGSettings>(); }
};
