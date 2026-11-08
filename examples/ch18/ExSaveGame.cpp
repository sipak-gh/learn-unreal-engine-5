// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSave, Log, All);

void AExSaveTester::BeginPlay()
{
	Super::BeginPlay();

	const int32 UserIndex = 0;
	UE_LOG(LogExSave, Log, TEXT("Slot %s exists before saving: %s"), *SlotName,
		UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex) ? TEXT("true") : TEXT("false"));

	// 1. Create an object of our save class and fill it in.
	UExSaveGame* ToSave = Cast<UExSaveGame>(UGameplayStatics::CreateSaveGameObject(UExSaveGame::StaticClass()));
	ToSave->Coins = 42;
	ToSave->PlayerName = TEXT("Ada");
	ToSave->Unlocked = { FName(TEXT("L_Level01")), FName(TEXT("L_Level02")) };
	ToSave->NotSaved = 7;

	// 2. Serialize it and write the file. Blocks until the file is written.
	const bool bSaved = UGameplayStatics::SaveGameToSlot(ToSave, SlotName, UserIndex);
	UE_LOG(LogExSave, Log, TEXT("Saved slot %s: %s"), *SlotName, bSaved ? TEXT("true") : TEXT("false"));

	// 3. Read the file into a brand-new object. Null if missing or not readable.
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	if (const UExSaveGame* Data = Cast<UExSaveGame>(Loaded))
	{
		UE_LOG(LogExSave, Log, TEXT("Loaded: %s, %d coins, %d unlocked (NotSaved = %d)"),
			*Data->PlayerName, Data->Coins, Data->Unlocked.Num(), Data->NotSaved);
		UE_LOG(LogExSave, Log, TEXT("Same object? %s"), Data == ToSave ? TEXT("yes") : TEXT("no"));
	}
	else
	{
		UE_LOG(LogExSave, Warning, TEXT("Could not load slot %s"), *SlotName);
	}
}

void AExSaveTester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bDeleteOnEndPlay)
	{
		const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
		UE_LOG(LogExSave, Log, TEXT("Deleted slot %s: %s"), *SlotName, bDeleted ? TEXT("true") : TEXT("false"));
	}

	Super::EndPlay(EndPlayReason);
}
