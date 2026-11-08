// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSaveMigration.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExMigration, Log, All);

namespace
{
	const TCHAR* MigrationSlot = TEXT("ExMigrationSlot");
}

bool UExVersionedSave::Migrate()
{
	const int32 Original = Version;

	if (Version < 2)
	{
		// v1 -> v2: Score became BestScore.
		BestScore = Score;
		Score = 0;
		Version = 2;
		UE_LOG(LogExMigration, Log, TEXT("Migrated v1 -> v2: BestScore = %d"), BestScore);
	}
	// A future "if (Version < 3) { ... Version = 3; }" goes here, after this step.

	return Version != Original;
}

void AExSaveMigrationTester::BeginPlay()
{
	Super::BeginPlay();

	// Pretend this file was written by last year's build: version 1, old field.
	UExVersionedSave* Old = Cast<UExVersionedSave>(UGameplayStatics::CreateSaveGameObject(UExVersionedSave::StaticClass()));
	Old->Version = 1;
	Old->Score = 250;
	UGameplayStatics::SaveGameToSlot(Old, MigrationSlot, 0);
	UE_LOG(LogExMigration, Log, TEXT("Wrote a version %d save with Score = %d"), Old->Version, Old->Score);

	// Today's build loads it.
	UExVersionedSave* Save = Cast<UExVersionedSave>(UGameplayStatics::LoadGameFromSlot(MigrationSlot, 0));
	if (!Save)
	{
		UE_LOG(LogExMigration, Warning, TEXT("No readable save - would start fresh"));
		return;
	}

	UE_LOG(LogExMigration, Log, TEXT("Loaded version %d (current is %d)"), Save->Version, UExVersionedSave::CurrentVersion);
	if (Save->Migrate())
	{
		// Write the upgraded data back, so the migration runs only once.
		UGameplayStatics::SaveGameToSlot(Save, MigrationSlot, 0);
	}

	UE_LOG(LogExMigration, Log, TEXT("Now version %d, BestScore %d"), Save->Version, Save->BestScore);
	UGameplayStatics::DeleteGameInSlot(MigrationSlot, 0);
}
