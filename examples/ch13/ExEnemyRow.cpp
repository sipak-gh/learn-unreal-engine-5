// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExEnemyRow.h"

void AExEnemyTableReader::BeginPlay()
{
	Super::BeginPlay();

	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: set Table in the Details panel"), *GetName());
		return;
	}

	// Row names, in the order they appear in the table.
	TArray<FString> Names;
	for (const FName& RowName : Table->GetRowNames())
	{
		Names.Add(RowName.ToString());
	}
	UE_LOG(LogTemp, Log, TEXT("%s has %d rows: %s"), *Table->GetName(), Names.Num(), *FString::Join(Names, TEXT(", ")));

	// The context string only appears in warnings, to say who was looking.
	static const TCHAR* Context = TEXT("ExEnemyRow");

	if (const FExEnemyRow* Bat = Table->FindRow<FExEnemyRow>(FName("Bat"), Context))
	{
		UE_LOG(LogTemp, Log, TEXT("Bat: %d HP, speed %.0f"), Bat->Health, Bat->Speed);
	}

	// A missing row returns nullptr; passing false skips the LogDataTable warning.
	const FExEnemyRow* Dragon = Table->FindRow<FExEnemyRow>(FName("Dragon"), Context, /*bWarnIfRowMissing=*/false);
	UE_LOG(LogTemp, Log, TEXT("Dragon: %s"), Dragon ? TEXT("found") : TEXT("no such row"));
}
