// Example for chapter 13 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "ExEnemyRow.generated.h"

/**
 * One row of an enemy table. The row's name (the first CSV column, "Bat") is
 * the key, so it is not a field here. Import ExEnemies.csv with this row type.
 */
USTRUCT(BlueprintType)
struct FExEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1"))
	int32 Health = 1;

	/** Units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float Speed = 100.0f;
};

/** Place one in a level, set Table to DT_ExEnemies, press Play. */
UCLASS()
class AExEnemyTableReader : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TObjectPtr<UDataTable> Table;

protected:
	virtual void BeginPlay() override;
};
