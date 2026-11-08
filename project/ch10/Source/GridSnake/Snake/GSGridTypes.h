// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GSGridTypes.generated.h"

/** A step on the grid. Up is towards the top of the screen (tile row - 1). */
UENUM(BlueprintType)
enum class EGSDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

namespace GSGrid
{
	/** The cell offset of one step. Tile rows grow downwards, so Up is Y - 1. */
	inline FIntPoint ToOffset(EGSDirection Direction)
	{
		switch (Direction)
		{
		case EGSDirection::Up:    return FIntPoint(0, -1);
		case EGSDirection::Down:  return FIntPoint(0, 1);
		case EGSDirection::Left:  return FIntPoint(-1, 0);
		case EGSDirection::Right: return FIntPoint(1, 0);
		}
		return FIntPoint::ZeroValue;
	}

	inline bool AreOpposite(EGSDirection A, EGSDirection B)
	{
		return ToOffset(A) + ToOffset(B) == FIntPoint::ZeroValue;
	}

	/**
	 * Rotation that turns a sprite drawn facing right (+X) to face Direction.
	 * Sprites lie in the X-Z plane, so turning them is a pitch (around Y).
	 */
	inline FRotator ToSpriteRotation(EGSDirection Direction)
	{
		switch (Direction)
		{
		case EGSDirection::Up:    return FRotator(90.0f, 0.0f, 0.0f);
		case EGSDirection::Down:  return FRotator(-90.0f, 0.0f, 0.0f);
		case EGSDirection::Left:  return FRotator(180.0f, 0.0f, 0.0f);
		case EGSDirection::Right: return FRotator::ZeroRotator;
		}
		return FRotator::ZeroRotator;
	}
}
