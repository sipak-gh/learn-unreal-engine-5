// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExReflectedThing.generated.h"

/**
 * A plain UObject (not an actor) with a few reflected members. Nothing places it
 * in a level; AExReflectionDemo creates one at runtime and inspects it.
 */
UCLASS(BlueprintType)
class UExReflectedThing : public UObject
{
	GENERATED_BODY()

public:
	UExReflectedThing();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thing", meta = (ClampMin = "0"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thing")
	FString Label = TEXT("Hello");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thing")
	TArray<FName> Tags;

	/** Not a UPROPERTY: invisible to reflection, the editor, Blueprints and the GC. */
	int32 Hidden = 42;

	UFUNCTION(BlueprintCallable, Category = "Thing")
	FString Describe() const;
};

/** Drop it into any level and press Play: it lists UExReflectedThing's members. */
UCLASS()
class AExReflectionDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	/** UPROPERTY so that the garbage collector knows this actor keeps Thing alive. */
	UPROPERTY()
	TObjectPtr<UExReflectedThing> Thing;
};
