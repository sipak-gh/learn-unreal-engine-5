// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpritePulse.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

AExSpritePulse::AExSpritePulse()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	SetRootComponent(Sprite);

	// Sprites lie in the X-Z plane: +X is to the right of the sprite.
	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Sprite);
	Flipbook->SetRelativeLocation(FVector(24.0f, 0.0f, 0.0f));
}

void AExSpritePulse::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s pulses %s <-> %s, %.1f per second"), *GetName(),
		*ColorA.ToString(), *ColorB.ToString(), PulsesPerSecond);
}

void AExSpritePulse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;

	// Sin runs from -1 to 1; remap it to 0..1 for the blend.
	const float Alpha = 0.5f + 0.5f * FMath::Sin(RunningTime * UE_TWO_PI * PulsesPerSecond);
	const FLinearColor Color = FLinearColor::LerpUsingHSV(ColorA, ColorB, Alpha);

	Sprite->SetSpriteColor(Color);
	Flipbook->SetSpriteColor(Color);
}
