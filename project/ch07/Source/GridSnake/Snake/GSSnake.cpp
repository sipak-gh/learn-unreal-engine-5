// GridSnake - course project for "Learn Unreal Engine 5".

#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"

AGSSnake::AGSSnake()
{
	PrimaryActorTick.bCanEverTick = false;

	// A grid pawn never turns with the controller; we rotate the head sprite ourselves.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->InitBoxExtent(FVector(4.0f, 16.0f, 4.0f));
	Collision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(Collision);

	Head = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Head"));
	Head->SetupAttachment(Collision);
	// In front of gems (+5) and the tile map (0).
	Head->SetRelativeLocation(FVector(0.0f, 10.0f, 0.0f));
	Head->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSSnake::BeginPlay()
{
	Super::BeginPlay();

	// Level actors already exist when a spawned pawn begins play.
	Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass()));
	if (!Board)
	{
		UE_LOG(LogGridSnake, Error, TEXT("%s: no GSBoard in this level - place one"), *GetName());
		return;
	}

	CurrentDirection = Board->GetStartDirection();
	SnapToBoard();
	UpdateHeadRotation();
}

void AGSSnake::SnapToBoard()
{
	HeadCell = Board->WorldToCell(GetActorLocation());
	if (!Board->IsInside(HeadCell))
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s spawned outside the board at %s; using cell (1, 1)"),
			*GetName(), *GetActorLocation().ToString());
		HeadCell = FIntPoint(1, 1);
	}

	// The Player Start's rotation would otherwise tilt the sprites out of the X-Z plane.
	SetActorLocationAndRotation(Board->CellToWorld(HeadCell), FRotator::ZeroRotator);
	UE_LOG(LogGridSnake, Log, TEXT("Snake starts at cell %s facing %s"),
		*HeadCell.ToString(), *UEnum::GetValueAsString(CurrentDirection));
}

void AGSSnake::UpdateHeadRotation()
{
	Head->SetRelativeRotation(GSGrid::ToSpriteRotation(CurrentDirection));
}
