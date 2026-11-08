// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GSSettings.generated.h"

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

	/** Seconds between two snake steps unless a level overrides it. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0.02", Units = "s"))
	float DefaultStepInterval = 0.15f;

	/** Score awarded for each gem. */
	UPROPERTY(Config, EditAnywhere, Category = "Rules", meta = (ClampMin = "0"))
	int32 PointsPerGem = 100;

	/** The class default object holds the values loaded from the .ini files. */
	static const UGSSettings* Get() { return GetDefault<UGSSettings>(); }
};
