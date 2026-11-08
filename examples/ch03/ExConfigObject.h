// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExConfigObject.generated.h"

/**
 * Config = Game without DefaultConfig: SaveConfig() writes to the per-user
 * Saved/Config/<Platform>/Game.ini, not to the project's Config/DefaultGame.ini.
 */
UCLASS(Config = Game)
class UExConfigObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config)
	int32 HighScore = 0;
};

/** Drop it into a level and press Play several times: HighScore goes up by one each time. */
UCLASS()
class AExConfigDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
