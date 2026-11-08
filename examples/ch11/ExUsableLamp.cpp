// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExUsableLamp.h"
#include "Components/PointLightComponent.h"

AExUsableLamp::AExUsableLamp()
{
	PrimaryActorTick.bCanEverTick = false;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	SetRootComponent(Light);
	// A Static light is baked into the level; a Movable one can be switched freely.
	Light->SetMobility(EComponentMobility::Movable);
}

void AExUsableLamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Light->SetVisibility(bOn);
}

void AExUsableLamp::Use_Implementation(AActor* User)
{
	bOn = !bOn;
	Light->SetVisibility(bOn);
	UE_LOG(LogTemp, Log, TEXT("%s switched %s by %s"), *GetName(), bOn ? TEXT("on") : TEXT("off"), *GetNameSafe(User));
}
