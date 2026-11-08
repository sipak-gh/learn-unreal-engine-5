// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSGem.h"
#include "GridSnake.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbookComponent.h"

AGSGem::AGSGem()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Root);
	// Sprites draw in the X-Z plane. A small +Y offset puts the gem in front of
	// the tile map (the camera looks down -Y from +Y; see chapter 6).
	Flipbook->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSGem::BeginPlay()
{
	Super::BeginPlay();

	BobPhase = FMath::FRandRange(0.0f, UE_TWO_PI);
	UE_LOG(LogGridSnake, Verbose, TEXT("%s spawned at %s"), *GetName(), *GetActorLocation().ToString());
}

void AGSGem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;
	const float Offset = FMath::Sin(RunningTime * BobSpeed + BobPhase) * BobHeight;

	FVector Location = Flipbook->GetRelativeLocation();
	Location.Z = Offset;
	Flipbook->SetRelativeLocation(Location);
}
