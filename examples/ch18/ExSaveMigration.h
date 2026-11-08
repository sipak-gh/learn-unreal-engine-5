// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "ExSaveMigration.generated.h"

/**
 * Version 1 stored "Score". Version 2 renamed it to "BestScore".
 * Score stays as a legacy field so that version 1 files can still be read.
 */
UCLASS()
class UExVersionedSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 2;

	/** 0 = written before versioning existed. Set to CurrentVersion before saving. */
	UPROPERTY()
	int32 Version = 0;

	/** Version 1 only. Read during migration, then cleared. */
	UPROPERTY()
	int32 Score = 0;

	/** Version 2 and later. */
	UPROPERTY()
	int32 BestScore = 0;

	/** Upgrades this object step by step. Returns true if anything changed. */
	bool Migrate();
};

/** Writes a version 1 file, then loads and migrates it. Drop it in a level and press Play. */
UCLASS()
class AExSaveMigrationTester : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
