// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpinner.h"
#include "PaperSpriteComponent.h"

AExSpinner::AExSpinner()
{
	// Without this, Tick is never called, however you override it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
}

void AExSpinner::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s spins at %.1f degrees per second"), *GetName(), DegreesPerSecond);
}

void AExSpinner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// FRotator is (Pitch, Yaw, Roll). Pitch turns about Y, the axis the 2D camera looks along.
	// Multiplying by DeltaSeconds makes the speed independent of the frame rate.
	AddActorLocalRotation(FRotator(DegreesPerSecond * DeltaSeconds, 0.0f, 0.0f));
}
