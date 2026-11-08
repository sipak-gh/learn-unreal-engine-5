// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"

/** Log category for all GridSnake gameplay code. Filter the Output Log with "LogGridSnake". */
DECLARE_LOG_CATEGORY_EXTERN(LogGridSnake, Log, All);

/**
 * The "Grid" trace channel from Project Settings > Engine > Collision. Anything
 * that blocks it is a cell the snake cannot enter. Custom channels are numbered
 * in the order they were created, so this must match DefaultEngine.ini.
 */
#define ECC_GSGrid ECC_GameTraceChannel1
