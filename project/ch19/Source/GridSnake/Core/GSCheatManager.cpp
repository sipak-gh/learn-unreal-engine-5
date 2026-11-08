// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSCheatManager.h"
#include "GridSnake.h"
#include "Core/GSGameMode.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/GSGem.h"
#include "Save/GSSaveSubsystem.h"
#include "Snake/GSSnake.h"

namespace
{
	AGSSnake* GetSnake(UCheatManager& Cheats)
	{
		const APlayerController* PlayerController = Cheats.GetPlayerController();
		return PlayerController ? Cast<AGSSnake>(PlayerController->GetPawn()) : nullptr;
	}
}

void UGSCheatManager::GSGrow(int32 Amount)
{
	if (AGSSnake* Snake = GetSnake(*this))
	{
		Snake->Grow(Amount);
		UE_LOG(LogGridSnake, Log, TEXT("GSGrow: +%d segments"), Amount);
	}
}

void UGSCheatManager::GSCollectAll()
{
	AGSSnake* Snake = GetSnake(*this);
	if (!Snake)
	{
		return;
	}

	// Collect into an array first: collecting destroys the gem, and an actor
	// iterator should not run while the actors it visits are being destroyed.
	TArray<AGSGem*> Gems;
	for (TActorIterator<AGSGem> It(GetWorld()); It; ++It)
	{
		Gems.Add(*It);
	}

	for (AGSGem* Gem : Gems)
	{
		IGSSnakeInteractable::Execute_OnSnakeEntered(Gem, Snake);
	}
	UE_LOG(LogGridSnake, Log, TEXT("GSCollectAll: %d gem(s)"), Gems.Num());
}

void UGSCheatManager::GSWin()
{
	AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>();
	AGSSnake* Snake = GetSnake(*this);
	if (GameMode && Snake)
	{
		GameMode->OnExitReached(Snake);
	}
}

void UGSCheatManager::GSUnlockAll()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	if (!LevelList || LevelList->Num() == 0)
	{
		return;
	}

	UGSSaveSubsystem* SaveSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGSSaveSubsystem>();
	SaveSubsystem->UnlockLevel(LevelList->Num() - 1);
	SaveSubsystem->Save();
	UE_LOG(LogGridSnake, Log, TEXT("GSUnlockAll: %d level(s) unlocked"), LevelList->Num());
}
