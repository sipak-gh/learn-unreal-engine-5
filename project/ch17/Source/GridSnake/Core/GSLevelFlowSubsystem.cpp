// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSLevelFlowSubsystem.h"
#include "GridSnake.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/GSSettings.h"
#include "Data/GSLevelList.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GridSnakeLevelFlow"

namespace
{
	/**
	 * The loading screen is drawn by the movie player on another thread while the
	 * game thread is busy loading, so it must be plain Slate: no UObjects, no UMG.
	 */
	TSharedRef<SWidget> MakeLoadingScreen(const FText& Message)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Black)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Message)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
				.ColorAndOpacity(FLinearColor::White)
			];
	}
}

void UGSLevelFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// PreLoadMap fires on the game thread just before a map starts loading.
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UGSLevelFlowSubsystem::HandlePreLoadMap);
	UE_LOG(LogGridSnake, Log, TEXT("Level flow subsystem initialised"));
}

void UGSLevelFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);

	Super::Deinitialize();
}

void UGSLevelFlowSubsystem::StartRun(int32 LevelIndex)
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FGSLevelInfo* Level = LevelList ? LevelList->GetLevel(LevelIndex) : nullptr;
	if (!Level)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("StartRun: no level %d"), LevelIndex);
		return;
	}

	RunScore = 0;
	CurrentLevelIndex = LevelIndex;
	OpenLevelWithFade(Level->Map);
}

bool UGSLevelFlowSubsystem::CompleteLevel(int32 LevelScore)
{
	RunScore += LevelScore;
	UE_LOG(LogGridSnake, Log, TEXT("Level %d complete: +%d, run score %d"), CurrentLevelIndex, LevelScore, RunScore);
	return HasNextLevel();
}

bool UGSLevelFlowSubsystem::HasNextLevel() const
{
	// An unlisted map has index INDEX_NONE (-1), so its "next level" is the first one.
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	return LevelList && LevelList->GetLevel(CurrentLevelIndex + 1) != nullptr;
}

void UGSLevelFlowSubsystem::OpenNextLevel()
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FGSLevelInfo* Next = LevelList ? LevelList->GetLevel(CurrentLevelIndex + 1) : nullptr;
	if (!Next)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("OpenNextLevel: level %d was the last one"), CurrentLevelIndex);
		return;
	}

	++CurrentLevelIndex;
	OpenLevelWithFade(Next->Map);
}

void UGSLevelFlowSubsystem::RestartLevel()
{
	if (const FGSLevelInfo* Level = GetCurrentLevelInfo())
	{
		OpenLevelWithFade(Level->Map);
		return;
	}

	// Not a listed level (a test map, say): reload it by name.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetGameInstance(), /*bRemovePrefixString=*/true);
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*LevelName));
}

void UGSLevelFlowSubsystem::ReturnToMainMenu()
{
	CurrentLevelIndex = INDEX_NONE;
	OpenLevelWithFade(UGSSettings::Get()->MainMenuMap);
}

void UGSLevelFlowSubsystem::NotifyLevelStarted(UWorld* World)
{
	bTransitioning = false;

	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	const FName LevelId(*UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString=*/true));
	CurrentLevelIndex = LevelList ? LevelList->FindLevelIndex(LevelId) : INDEX_NONE;
	UE_LOG(LogGridSnake, Log, TEXT("Level %s started (index %d, run score %d)"), *LevelId.ToString(), CurrentLevelIndex, RunScore);

	// Start black and fade in, so the new level never pops onto the screen.
	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(World))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/false);
		}
	}
}

const FGSLevelInfo* UGSLevelFlowSubsystem::GetCurrentLevelInfo() const
{
	const UGSLevelList* LevelList = UGSSettings::GetLevelList();
	return LevelList ? LevelList->GetLevel(CurrentLevelIndex) : nullptr;
}

void UGSLevelFlowSubsystem::OpenLevelWithFade(const TSoftObjectPtr<UWorld>& Map)
{
	if (bTransitioning)
	{
		return;
	}

	bTransitioning = true;
	PendingMap = Map;

	// The pause menu can restart or quit the level while the game is paused.
	// The pause menu already hides the level, so skip the fade and go now.
	if (UGameplayStatics::IsGamePaused(GetGameInstance()))
	{
		OpenPendingLevel();
		return;
	}

	if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeSeconds, FLinearColor::Black, /*bShouldFadeAudio=*/true, /*bHoldWhenFinished=*/true);
		}
	}

	// The game instance's timer manager outlives every world, unlike GetWorldTimerManager().
	GetGameInstance()->GetTimerManager().SetTimer(TransitionTimer,
		FTimerDelegate::CreateUObject(this, &UGSLevelFlowSubsystem::OpenPendingLevel), FadeSeconds, /*bLoop=*/false);
}

void UGSLevelFlowSubsystem::OpenPendingLevel()
{
	UE_LOG(LogGridSnake, Log, TEXT("Opening %s"), *PendingMap.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), PendingMap);
}

void UGSLevelFlowSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// The movie player only runs in standalone and packaged games, never in
	// Play In Editor, so test the loading screen with Standalone Game.
	if (IsRunningDedicatedServer() || !IsMoviePlayerEnabled())
	{
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.MinimumLoadingScreenDisplayTime = 1.0f;
	const FGSLevelInfo* Level = GetCurrentLevelInfo();
	const FText Message = Level
		? FText::Format(LOCTEXT("LoadingLevel", "Loading {0}..."), Level->DisplayName)
		: LOCTEXT("Loading", "Loading...");
	LoadingScreen.WidgetLoadingScreen = MakeLoadingScreen(Message);
	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

#undef LOCTEXT_NAMESPACE
