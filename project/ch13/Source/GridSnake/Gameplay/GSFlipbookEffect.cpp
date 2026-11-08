// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSFlipbookEffect.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSFlipbookEffect::AGSFlipbookEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	SetRootComponent(Flipbook);
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flipbook->SetLooping(false);
}

void AGSFlipbookEffect::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook->GetFlipbook())
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no flipbook"), *GetName());
		Destroy();
		return;
	}

	// Depth (Y): in front of gems and doors (+5) and the body (+8), behind the head (+10).
	AddActorWorldOffset(FVector(0.0f, 9.0f, 0.0f));
	Flipbook->OnFinishedPlaying.AddDynamic(this, &AGSFlipbookEffect::HandleFinishedPlaying);
	Flipbook->SetLooping(false);
	Flipbook->PlayFromStart();
}

void AGSFlipbookEffect::HandleFinishedPlaying()
{
	Destroy();
}
