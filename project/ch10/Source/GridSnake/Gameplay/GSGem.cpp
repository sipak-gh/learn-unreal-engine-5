// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSGem.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"

AGSGem::AGSGem()
{
	PrimaryActorTick.bCanEverTick = true;

	// WorldDynamic trigger: ignores everything except the snake (a Pawn).
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Trigger);
	// Sprites draw in the X-Z plane. A small +Y offset puts the gem in front of
	// the tile map (the camera looks down -Y from +Y; see chapter 6).
	Flipbook->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Flipbook->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSGem::BeginPlay()
{
	Super::BeginPlay();

	// Hand-placed gems are rarely exactly on a cell centre; the snake's head is.
	if (const AGSBoard* Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass())))
	{
		Board->SnapActorToCell(this);
	}

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
