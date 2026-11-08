// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GSLevelList.generated.h"

class UCurveFloat;

/** Everything the game needs to know about one level. */
USTRUCT(BlueprintType)
struct FGSLevelInfo
{
	GENERATED_BODY()

	/** Soft reference: naming a map here does not load it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Map;

	/** Shown on the HUD and in level select. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FText DisplayName;

	/** Seconds per step at the starting length. Smaller is faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0.02", Units = "s"))
	float StepInterval = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (ClampMin = "0"))
	int32 PointsPerGem = 100;

	/** Optional. X = snake length, Y = step interval multiplier (below 1 speeds the snake up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TObjectPtr<UCurveFloat> SpeedCurve;

	/** Stable key for this level (the map's asset name, e.g. L_Level01). Used for saving in chapter 18. */
	FName GetLevelId() const { return FName(*Map.GetAssetName()); }
};

/**
 * The ordered list of levels: the game's progression, edited as data.
 * DA_LevelList is the one instance; UGSSettings points at it.
 */
UCLASS(BlueprintType)
class UGSLevelList : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TArray<FGSLevelInfo> Levels;

	/** Index of the level whose map has this asset name, or INDEX_NONE. */
	int32 FindLevelIndex(FName LevelId) const;

	/** nullptr if Index is out of range. */
	const FGSLevelInfo* GetLevel(int32 Index) const;

	int32 Num() const { return Levels.Num(); }
};
