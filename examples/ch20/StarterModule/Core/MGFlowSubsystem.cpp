// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "Core/MGFlowSubsystem.h"
#include "MyGame.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/MGSettings.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UMGFlowSubsystem::OpenLevelByIndex(int32 LevelIndex)
{
	const TArray<TSoftObjectPtr<UWorld>>& Levels = UMGSettings::Get()->Levels;
	if (!Levels.IsValidIndex(LevelIndex))
	{
		UE_LOG(LogMyGame, Warning, TEXT("OpenLevelByIndex: no level %d (the list has %d)"), LevelIndex, Levels.Num());
		return;
	}

	CurrentLevelIndex = LevelIndex;
	OpenLevelWithFade(Levels[LevelIndex]);
}

bool UMGFlowSubsystem::OpenNextLevel()
{
	if (!UMGSettings::Get()->Levels.IsValidIndex(CurrentLevelIndex + 1))
	{
		return false;
	}

	OpenLevelByIndex(CurrentLevelIndex + 1);
	return true;
}

void UMGFlowSubsystem::RestartLevel()
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetGameInstance(), /*bRemovePrefixString=*/true);
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*LevelName));
}

void UMGFlowSubsystem::ReturnToMainMenu()
{
	CurrentLevelIndex = INDEX_NONE;
	OpenLevelWithFade(UMGSettings::Get()->MainMenuMap);
}

void UMGFlowSubsystem::NotifyLevelStarted(UWorld* World)
{
	bTransitioning = false;

	// Work out the index from the map's name, so Play In Editor on any listed map works.
	const FString MapName = UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true);
	const TArray<TSoftObjectPtr<UWorld>>& Levels = UMGSettings::Get()->Levels;
	CurrentLevelIndex = Levels.IndexOfByPredicate([&MapName](const TSoftObjectPtr<UWorld>& Level)
	{
		return Level.GetAssetName() == MapName;
	});
	UE_LOG(LogMyGame, Log, TEXT("Level %s started (index %d)"), *MapName, CurrentLevelIndex);

	// Start black and fade in, so the new level never pops onto the screen.
	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(World))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, UMGSettings::Get()->FadeSeconds,
				FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/false);
		}
	}
}

void UMGFlowSubsystem::OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map)
{
	if (bTransitioning)
	{
		return;
	}

	if (Map.IsNull())
	{
		UE_LOG(LogMyGame, Warning, TEXT("OpenLevelWithFade: no map set - fill in Project Settings > Game > My Game"));
		return;
	}

	bTransitioning = true;
	PendingMap = Map;

	const float FadeSeconds = UMGSettings::Get()->FadeSeconds;
	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds,
				FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);
		}
	}

	// The game instance's timer manager outlives every world, unlike GetWorldTimerManager().
	if (FadeSeconds > 0.0f)
	{
		GetGameInstance()->GetTimerManager().SetTimer(TransitionTimer,
			FTimerDelegate::CreateUObject(this, &UMGFlowSubsystem::OpenPendingLevel), FadeSeconds, /*bLoop=*/false);
	}
	else
	{
		OpenPendingLevel();
	}
}

void UMGFlowSubsystem::OpenPendingLevel()
{
	UE_LOG(LogMyGame, Log, TEXT("Opening %s"), *PendingMap.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), PendingMap);
}
