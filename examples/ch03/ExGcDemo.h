// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "ExGcDemo.generated.h"

/** A tiny object that logs when the garbage collector destroys it. */
UCLASS()
class UExGcPayload : public UObject
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;
};

/**
 * Creates three payloads and keeps them in three kinds of pointer, forces a
 * garbage collection, then checks which survived. Drop it into a level and Play.
 */
UCLASS()
class AExGcDemo : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	void CheckPointers();

	/** A strong, GC-visible reference: keeps its object alive. */
	UPROPERTY()
	TObjectPtr<UExGcPayload> Kept;

	/** A weak reference: does not keep its object alive, but knows when it is gone. */
	TWeakObjectPtr<UExGcPayload> Weak;

	/** A raw pointer without UPROPERTY: the GC cannot see it. Never dereference it after a GC. */
	UExGcPayload* Raw = nullptr;

	FTimerHandle CheckTimer;
};
