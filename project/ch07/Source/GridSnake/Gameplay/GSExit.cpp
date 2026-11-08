// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSExit.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSExit::AGSExit()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Door = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Door"));
	Door->SetupAttachment(Root);
	Door->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Door->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSExit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor whenever the actor is placed, moved or edited, so the
	// level shows the right door state without pressing Play.
	UpdateVisuals();
}

void AGSExit::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGridSnake, Log, TEXT("%s starts %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::SetOpen(bool bNewOpen)
{
	if (bIsOpen == bNewOpen)
	{
		return;
	}

	bIsOpen = bNewOpen;
	UpdateVisuals();
	UE_LOG(LogGridSnake, Log, TEXT("%s is now %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::UpdateVisuals()
{
	UPaperFlipbook* Wanted = bIsOpen ? OpenFlipbook : ClosedFlipbook;
	if (Wanted)
	{
		Door->SetFlipbook(Wanted);
		Door->PlayFromStart();
	}
}
