// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExPixelSnap.generated.h"

/**
 * A Blueprint function library: static functions that show up in every
 * Blueprint's node menu under "Ex|Pixel Art". No instance is ever placed.
 */
UCLASS()
class UExPixelSnapLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Rounds X and Z (the Paper2D sprite plane) to the nearest multiple of Unit.
	 * Y is depth and stays as it is. Unit <= 0 returns the location unchanged.
	 */
	UFUNCTION(BlueprintPure, Category = "Ex|Pixel Art")
	static FVector SnapToPixelGrid(FVector Location, float Unit = 1.0f);

	/** Logs a few test calls. Call it from a Level Blueprint's BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Ex|Pixel Art")
	static void LogSnapExamples();
};
