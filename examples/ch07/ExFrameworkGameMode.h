// Learn Unreal Engine 5 - chapter 7 example.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ExFrameworkGameMode.generated.h"

/**
 * A game mode that changes nothing: every override logs and calls Super, so the
 * Output Log shows the order in which a level starts and a player joins.
 * Use it as World Settings > GameMode Override.
 */
UCLASS()
class AExFrameworkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** First call on a new game mode, before any other actor is initialised for play. */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** A player controller exists and has joined. Safe to call functions on it. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Decides what a new player starts as. The default restarts the player, which spawns a pawn. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Spawns the Default Pawn Class at the start spot. */
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	/** Starts the match: every actor's BeginPlay runs inside this call. */
	virtual void StartPlay() override;
};
