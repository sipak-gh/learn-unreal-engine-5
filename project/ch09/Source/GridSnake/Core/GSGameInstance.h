// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GSGameInstance.generated.h"

/**
 * Created when the game starts and destroyed when it quits: it survives every
 * level change. Later chapters hang level progression and saving off it.
 */
UCLASS()
class UGSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Console: GSDumpClass GSSettings - lists a class's reflected properties and functions. */
	UFUNCTION(Exec)
	void GSDumpClass(const FString& ClassName);
};
