// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GSMenuGameMode.generated.h"

/**
 * The game mode of L_MainMenu (set as the map's GameMode Override in World
 * Settings). There is nothing to play, so the player never gets a pawn.
 */
UCLASS()
class AGSMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGSMenuGameMode();

	virtual void StartPlay() override;

protected:
	/** No pawn in the menu: the player controller only shows widgets. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
};
