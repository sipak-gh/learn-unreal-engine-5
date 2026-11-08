// Learn Unreal Engine 5 - chapter 7 example.

#include "ExWhoIsWho.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogExWho, Log, All);

namespace
{
	/** "Name (Class)", or "none" for a null pointer. */
	FString Describe(const UObject* Object)
	{
		return Object ? FString::Printf(TEXT("%s (%s)"), *Object->GetName(), *Object->GetClass()->GetName())
		              : FString(TEXT("none"));
	}
}

void AExWhoIsWho::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	// Lives for the whole run of the game, across level changes.
	UE_LOG(LogExWho, Log, TEXT("GetGameInstance() -> %s"), *Describe(GetGameInstance()));

	// Only exists where the rules run: the server, or a single-player game.
	UE_LOG(LogExWho, Log, TEXT("World->GetAuthGameMode() -> %s"), *Describe(World->GetAuthGameMode()));

	// Exists everywhere: the state all players may read.
	UE_LOG(LogExWho, Log, TEXT("World->GetGameState() -> %s"), *Describe(World->GetGameState()));

	APlayerController* PC = World->GetFirstPlayerController();
	UE_LOG(LogExWho, Log, TEXT("World->GetFirstPlayerController() -> %s"), *Describe(PC));
	if (!PC)
	{
		return;
	}

	// Per-player data that everyone may read (name, score in a multiplayer game).
	UE_LOG(LogExWho, Log, TEXT("PC->GetPlayerState<APlayerState>() -> %s"), *Describe(PC->GetPlayerState<APlayerState>()));

	// The body the controller drives right now. May be null (spectating, dead).
	UE_LOG(LogExWho, Log, TEXT("PC->GetPawn() -> %s"), *Describe(PC->GetPawn()));

	// The old canvas HUD actor; UMG widgets (chapter 15) do not need it.
	UE_LOG(LogExWho, Log, TEXT("PC->GetHUD() -> %s"), *Describe(PC->GetHUD()));

	// Decides what the player sees each frame; follows the view target.
	UE_LOG(LogExWho, Log, TEXT("PC->PlayerCameraManager -> %s"), *Describe(PC->PlayerCameraManager.Get()));
	UE_LOG(LogExWho, Log, TEXT("PC->GetViewTarget() -> %s"), *Describe(PC->GetViewTarget()));
}
