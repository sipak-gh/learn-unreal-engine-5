// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExAsyncSave.h"
#include "ExSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogExAsyncSave, Log, All);

void AExAsyncSave::BeginPlay()
{
	Super::BeginPlay();

	UExSaveGame* ToSave = Cast<UExSaveGame>(UGameplayStatics::CreateSaveGameObject(UExSaveGame::StaticClass()));
	ToSave->Coins = 7;
	ToSave->PlayerName = TEXT("Grace");

	// GFrameCounter counts engine frames; it shows when each line runs.
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: calling AsyncSaveGameToSlot"), GFrameCounter);

	// The object is serialized now, on the game thread. The file is written on
	// another thread, and the delegate runs back on the game thread afterwards.
	UGameplayStatics::AsyncSaveGameToSlot(ToSave, SlotName, /*UserIndex=*/0,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &AExAsyncSave::HandleSaved));

	// Safe: the data was already copied out of ToSave.
	ToSave->Coins = 999;
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: after the call, BeginPlay continues"), GFrameCounter);
}

void AExAsyncSave::HandleSaved(const FString& InSlotName, const int32 InUserIndex, bool bSuccess)
{
	UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: save to %s (user %d) finished: %s"),
		GFrameCounter, *InSlotName, InUserIndex, bSuccess ? TEXT("true") : TEXT("false"));

	UGameplayStatics::AsyncLoadGameFromSlot(InSlotName, InUserIndex,
		FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AExAsyncSave::HandleLoaded));
}

void AExAsyncSave::HandleLoaded(const FString& InSlotName, const int32 InUserIndex, USaveGame* Loaded)
{
	if (const UExSaveGame* Data = Cast<UExSaveGame>(Loaded))
	{
		UE_LOG(LogExAsyncSave, Log, TEXT("Frame %llu: loaded %s with %d coins"), GFrameCounter, *Data->PlayerName, Data->Coins);
	}
	else
	{
		UE_LOG(LogExAsyncSave, Warning, TEXT("Frame %llu: nothing loaded from %s"), GFrameCounter, *InSlotName);
	}
}
