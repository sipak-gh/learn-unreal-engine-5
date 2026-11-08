// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExOneShotEffect.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFx, Log, All);

AExOneShotEffect::AExOneShotEffect()
{
	PrimaryActorTick.bCanEverTick = false; // the component ticks on its own

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	SetRootComponent(Flipbook);
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flipbook->SetLooping(false);

	// Safety net: even if the flipbook never finishes (say, other code switches
	// Looping back on), the actor is destroyed after this many seconds.
	InitialLifeSpan = 5.0f;
}

void AExOneShotEffect::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook->GetFlipbook())
	{
		UE_LOG(LogExFx, Warning, TEXT("%s has no flipbook, destroying"), *GetName());
		Destroy();
		return;
	}

	Flipbook->OnFinishedPlaying.AddDynamic(this, &AExOneShotEffect::HandleFinished);
	Flipbook->SetLooping(false);
	Flipbook->PlayFromStart();
	StartTime = GetWorld()->GetTimeSeconds();
}

void AExOneShotEffect::HandleFinished()
{
	UE_LOG(LogExFx, Log, TEXT("%s finished after %.2f s (flipbook length %.2f s), destroying"), *GetName(),
		GetWorld()->GetTimeSeconds() - StartTime, Flipbook->GetFlipbookLength());
	Destroy();
}
