// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MGGameMode.generated.h"

/**
 * The rules of a level. Make a Blueprint child (BP_MGGameMode), set its Default Pawn
 * Class to your BP_MGPawn2D, and select it in Project Settings > Maps & Modes.
 */
UCLASS()
class AMGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMGGameMode();

	virtual void StartPlay() override;
};
