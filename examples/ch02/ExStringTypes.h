// Chapter 2 example: FString, FName and FText, and converting between them.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExStringTypes.generated.h"

/** Drop into a level and press Play: logs one line per string operation. */
UCLASS()
class AExStringTypes : public AActor
{
	GENERATED_BODY()

public:
	AExStringTypes();

protected:
	virtual void BeginPlay() override;
};
