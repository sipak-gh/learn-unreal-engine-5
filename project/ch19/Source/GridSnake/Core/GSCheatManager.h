// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GSCheatManager.generated.h"

/**
 * Console commands for testing. The player controller creates this object in
 * development builds only (Shipping builds have no cheat manager), so cheats
 * cannot ship by accident. Type the function name in the console: GSGrow 5
 */
UCLASS()
class UGSCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Adds segments to the snake. */
	UFUNCTION(Exec)
	void GSGrow(int32 Amount = 5);

	/** Collects every gem in the level, which opens the exit. */
	UFUNCTION(Exec)
	void GSCollectAll();

	/** Finishes the level as if the snake had reached the open exit. */
	UFUNCTION(Exec)
	void GSWin();

	/** Unlocks every level in the level list and saves. */
	UFUNCTION(Exec)
	void GSUnlockAll();
};
