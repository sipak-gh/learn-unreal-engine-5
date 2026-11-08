// Learn Unreal Engine 5 - chapter 7 example.

#include "ExFrameworkGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFramework, Log, All);

void AExFrameworkGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UE_LOG(LogExFramework, Log, TEXT("InitGame map=%s"), *MapName);
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AExFrameworkGameMode::PostLogin(APlayerController* NewPlayer)
{
	// Log first: Super::PostLogin is what calls HandleStartingNewPlayer.
	UE_LOG(LogExFramework, Log, TEXT("PostLogin %s"), *GetNameSafe(NewPlayer));
	Super::PostLogin(NewPlayer);
}

void AExFrameworkGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UE_LOG(LogExFramework, Log, TEXT("HandleStartingNewPlayer %s"), *GetNameSafe(NewPlayer));
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

APawn* AExFrameworkGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// Log after Super, so we can print the pawn it spawned.
	APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	UE_LOG(LogExFramework, Log, TEXT("SpawnDefaultPawnFor -> %s at %s (start spot %s)"),
		*GetNameSafe(Pawn),
		Pawn ? *Pawn->GetActorLocation().ToString() : TEXT("-"),
		*GetNameSafe(StartSpot));
	return Pawn;
}

void AExFrameworkGameMode::StartPlay()
{
	UE_LOG(LogExFramework, Log, TEXT("StartPlay"));
	Super::StartPlay();
}
