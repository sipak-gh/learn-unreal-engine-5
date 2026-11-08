// GridSnake - course project for "Learn Unreal Engine 5".

#include "Save/GSSaveSubsystem.h"
#include "GridSnake.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GSSaveGame.h"

const FString UGSSaveSubsystem::SlotName(TEXT("GridSnake"));

void UGSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		// Cast returns nullptr if the file holds some other class (or is damaged).
		SaveGame = Cast<UGSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	}

	if (!SaveGame)
	{
		SaveGame = Cast<UGSSaveGame>(UGameplayStatics::CreateSaveGameObject(UGSSaveGame::StaticClass()));
		SaveGame->SaveVersion = UGSSaveGame::CurrentVersion;
		UE_LOG(LogGridSnake, Log, TEXT("No save game in slot %s - starting fresh"), *SlotName);
		return;
	}

	if (SaveGame->SaveVersion < UGSSaveGame::CurrentVersion)
	{
		// Upgrade older saves here, one version step at a time.
		SaveGame->SaveVersion = UGSSaveGame::CurrentVersion;
	}

	UE_LOG(LogGridSnake, Log, TEXT("Loaded save game: %d level(s) unlocked, best run %d"),
		SaveGame->UnlockedLevelCount, SaveGame->BestRunScore);
}

bool UGSSaveSubsystem::IsLevelUnlocked(int32 LevelIndex) const
{
	return LevelIndex >= 0 && LevelIndex < SaveGame->UnlockedLevelCount;
}

int32 UGSSaveSubsystem::GetUnlockedLevelCount() const
{
	return SaveGame->UnlockedLevelCount;
}

bool UGSSaveSubsystem::HasProgress() const
{
	return SaveGame->UnlockedLevelCount > 1;
}

int32 UGSSaveSubsystem::GetBestScore(FName LevelId) const
{
	const int32* Best = SaveGame->BestScores.Find(LevelId);
	return Best ? *Best : 0;
}

int32 UGSSaveSubsystem::GetBestRunScore() const
{
	return SaveGame->BestRunScore;
}

void UGSSaveSubsystem::UnlockLevel(int32 LevelIndex)
{
	SaveGame->UnlockedLevelCount = FMath::Max(SaveGame->UnlockedLevelCount, LevelIndex + 1);
}

bool UGSSaveSubsystem::SubmitLevelScore(FName LevelId, int32 Score)
{
	int32& Best = SaveGame->BestScores.FindOrAdd(LevelId);
	if (Score <= Best)
	{
		return false;
	}
	Best = Score;
	return true;
}

bool UGSSaveSubsystem::SubmitRunScore(int32 Score)
{
	if (Score <= SaveGame->BestRunScore)
	{
		return false;
	}
	SaveGame->BestRunScore = Score;
	return true;
}

float UGSSaveSubsystem::GetMusicVolume() const
{
	return SaveGame->MusicVolume;
}

float UGSSaveSubsystem::GetSfxVolume() const
{
	return SaveGame->SfxVolume;
}

void UGSSaveSubsystem::SetVolumes(float MusicVolume, float SfxVolume)
{
	SaveGame->MusicVolume = MusicVolume;
	SaveGame->SfxVolume = SfxVolume;
}

void UGSSaveSubsystem::ResetProgress()
{
	SaveGame->UnlockedLevelCount = 1;
	SaveGame->BestScores.Reset();
	SaveGame->BestRunScore = 0;
	Save();
}

void UGSSaveSubsystem::Save()
{
	// The object is serialized right away on the game thread; only the file
	// write happens in the background, so changing SaveGame afterwards is safe.
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SlotName, UserIndex,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UGSSaveSubsystem::HandleSaveFinished));
}

void UGSSaveSubsystem::HandleSaveFinished(const FString& InSlotName, const int32 InUserIndex, bool bSuccess)
{
	UE_LOG(LogGridSnake, Log, TEXT("Save to slot %s %s"), *InSlotName, bSuccess ? TEXT("succeeded") : TEXT("FAILED"));
}
