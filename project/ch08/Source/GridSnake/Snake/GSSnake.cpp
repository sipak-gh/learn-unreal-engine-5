// GridSnake - course project for "Learn Unreal Engine 5".

#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "Gameplay/GSBoard.h"
#include "InputActionValue.h"
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

void AGSSnake::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// DefaultInput.ini makes every input component an Enhanced Input component.
	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (SteerAction)
	{
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this, &AGSSnake::OnSteer);
	}
	else
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s has no SteerAction - set it in BP_GSSnake"), *GetName());
	}
}

namespace
{
	/**
	 * Turns a 2D input (keys, D-pad or stick) into one of four directions.
	 * Returns false for input too small to mean anything. On a perfect diagonal
	 * (two keys held) it prefers turning over going straight on.
	 */
	bool DirectionFromInput(const FVector2D& Input, EGSDirection Current, EGSDirection& OutDirection)
	{
		const double AbsX = FMath::Abs(Input.X);
		const double AbsY = FMath::Abs(Input.Y);
		if (FMath::Max(AbsX, AbsY) < 0.5)
		{
			return false;
		}

		bool bHorizontal = AbsX > AbsY;
		if (FMath::IsNearlyEqual(AbsX, AbsY))
		{
			const bool bMovingHorizontally = Current == EGSDirection::Left || Current == EGSDirection::Right;
			bHorizontal = !bMovingHorizontally;
		}

		if (bHorizontal)
		{
			OutDirection = Input.X > 0.0 ? EGSDirection::Right : EGSDirection::Left;
		}
		else
		{
			OutDirection = Input.Y > 0.0 ? EGSDirection::Up : EGSDirection::Down;
		}
		return true;
	}
}

void AGSSnake::OnSteer(const FInputActionValue& Value)
{
	EGSDirection NewDirection;
	if (!DirectionFromInput(Value.Get<FVector2D>(), CurrentDirection, NewDirection))
	{
		return;
	}

	// A snake cannot reverse into its own neck.
	if (NewDirection == CurrentDirection || GSGrid::AreOpposite(NewDirection, CurrentDirection))
	{
		return;
	}

	CurrentDirection = NewDirection;
	UpdateHeadRotation();
	UE_LOG(LogGridSnake, Log, TEXT("Steer: %s"), *UEnum::GetValueAsString(CurrentDirection));
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
