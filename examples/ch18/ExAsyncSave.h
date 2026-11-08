// Example for chapter 18 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.
// Uses UExSaveGame from ExSaveGame.h (same folder).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExAsyncSave.generated.h"

class USaveGame;

/**
 * Saves with AsyncSaveGameToSlot, then loads with AsyncLoadGameFromSlot.
 * The log shows that BeginPlay carries on before the save has finished.
 */
UCLASS()
class AExAsyncSave : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Save")
	FString SlotName = TEXT("ExAsyncSlot");

protected:
	virtual void BeginPlay() override;

private:
	// Plain C++ handlers: these delegates are not dynamic, so no UFUNCTION is needed.
	void HandleSaved(const FString& InSlotName, const int32 InUserIndex, bool bSuccess);
	void HandleLoaded(const FString& InSlotName, const int32 InUserIndex, USaveGame* Loaded);
};
