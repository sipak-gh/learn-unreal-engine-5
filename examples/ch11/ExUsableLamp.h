// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExUsable.h"
#include "ExUsableLamp.generated.h"

class UPointLightComponent;

/** A light that switches on and off when used. Implements IExUsable in C++. */
UCLASS()
class AExUsableLamp : public AActor, public IExUsable
{
	GENERATED_BODY()

public:
	AExUsableLamp();

	virtual void OnConstruction(const FTransform& Transform) override;

	// IExUsable. Override the _Implementation, never Use() itself.
	virtual void Use_Implementation(AActor* User) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditAnywhere, Category = "Example")
	bool bOn = true;
};
