// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExSaveGame.generated.h"

/** The data to save. Only UPROPERTY fields are written to the file. */
UCLASS()
class UExSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 Coins = 0;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	FString PlayerName;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	TArray<FName> Unlocked;

	/** Not a UPROPERTY: never saved, always 0 after loading. */
	int32 NotSaved = 0;
};

/**
 * Saves a UExSaveGame to slot "ExSlot" synchronously, loads it back and logs both
 * steps. Drop it in a level and press Play.
 */
UCLASS()
class AExSaveTester : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Save")
	FString SlotName = TEXT("ExSlot");

	/** Delete the file again in EndPlay, so every run starts without a save. */
	UPROPERTY(EditAnywhere, Category = "Save")
	bool bDeleteOnEndPlay = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
