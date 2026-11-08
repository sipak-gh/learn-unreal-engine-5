// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExDelegateZoo.generated.h"

// Single, native: one target, may return a value. Parameters: types only.
DECLARE_DELEGATE_RetVal_OneParam(int32, FExSquare, int32);

// Multicast, native: any number of targets, no return value.
DECLARE_MULTICAST_DELEGATE_OneParam(FExNotify, const FString&);

// Multicast, dynamic: reflected, so Blueprints can bind. Parameters: type, then name.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExScored, int32, Points);

/** Declares one delegate of each common kind and exercises it in BeginPlay. */
UCLASS()
class AExDelegateZoo : public AActor
{
	GENERATED_BODY()

public:
	/** Shows up as "Assign On Scored" / "Bind Event to On Scored" in Blueprints. */
	UPROPERTY(BlueprintAssignable, Category = "Example")
	FExScored OnScored;

protected:
	virtual void BeginPlay() override;

private:
	/** A plain member function: fine for AddUObject, no UFUNCTION needed. */
	void NativeListener(const FString& Message);

	/** Dynamic delegates find their target by name, so this must be a UFUNCTION. */
	UFUNCTION()
	void HandleScored(int32 Points);

	FExSquare Square;
	FExNotify OnNotify;
};
