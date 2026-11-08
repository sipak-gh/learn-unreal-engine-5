// GridSnake - course project for "Learn Unreal Engine 5".

#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Core/GSSettings.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "Gameplay/GSBoard.h"
#include "Gameplay/GSSnakeInteractable.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"
#include "PaperGroupedSpriteComponent.h"

namespace
{
	/** Depth (Y) of the body sprites: in front of gems (+5), behind the head (+10). */
	constexpr float BodyDepth = 8.0f;
}

AGSSnake::AGSSnake()
{
	PrimaryActorTick.bCanEverTick = true;

	// A grid pawn never turns with the controller; we rotate the head sprite ourselves.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// The head's box: a Pawn that only overlaps WorldDynamic triggers (gems,
	// exits). It never blocks anything; walls are found by asking the board.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->InitBoxExtent(FVector(4.0f, 16.0f, 4.0f));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Pawn);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	SetRootComponent(Collision);

	Head = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Head"));
	Head->SetupAttachment(Collision);
	// In front of gems (+5) and the tile map (0).
	Head->SetRelativeLocation(FVector(0.0f, 10.0f, 0.0f));
	Head->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	Body = CreateDefaultSubobject<UPaperGroupedSpriteComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
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

	// A quarter-cell box sits well inside the head's cell, never touching neighbours.
	const float HalfSize = Board->GetCellSize() * 0.25f;
	Collision->SetBoxExtent(FVector(HalfSize, 16.0f, HalfSize));

	StepInterval = UGSSettings::Get()->DefaultStepInterval;
	CurrentDirection = Board->GetStartDirection();
	PlaceOnBoard();
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

void AGSSnake::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bMoving)
	{
		return;
	}

	// Fixed-step movement: frames come at any rate, steps at exactly one per
	// StepInterval. A long frame may owe more than one step.
	StepAccumulator += DeltaSeconds;
	while (bMoving && StepAccumulator >= StepInterval)
	{
		StepAccumulator -= StepInterval;
		Step();
	}
}

void AGSSnake::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bAlive || !OtherActor)
	{
		return;
	}

	// The snake does not care what it entered, only that it can be entered.
	// Implements<>() and Execute_ work for C++ and Blueprint implementations alike.
	if (OtherActor->Implements<UGSSnakeInteractable>())
	{
		UE_LOG(LogGridSnake, Verbose, TEXT("Snake entered %s at cell %s"), *OtherActor->GetName(), *GetHeadCell().ToString());
		IGSSnakeInteractable::Execute_OnSnakeEntered(OtherActor, this);
	}
}

void AGSSnake::StartMoving()
{
	if (!bAlive || !Board)
	{
		return;
	}

	bMoving = true;
	StepAccumulator = 0.0f;
	UE_LOG(LogGridSnake, Log, TEXT("Snake starts moving %s, one step every %.2fs"),
		*UEnum::GetValueAsString(CurrentDirection), StepInterval);
}

void AGSSnake::StopMoving()
{
	bMoving = false;
	DirectionQueue.Reset();
}

void AGSSnake::Grow(int32 Amount)
{
	PendingGrowth += FMath::Max(Amount, 0);
}

void AGSSnake::GSGrow(int32 Amount)
{
	Grow(Amount);
	UE_LOG(LogGridSnake, Log, TEXT("GSGrow: %d segments pending"), PendingGrowth);
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
	// Compare against the direction the snake will have after the queued turns.
	const EGSDirection Reference = DirectionQueue.Num() > 0 ? DirectionQueue.Last() : CurrentDirection;

	EGSDirection NewDirection;
	if (DirectionFromInput(Value.Get<FVector2D>(), Reference, NewDirection))
	{
		QueueDirection(NewDirection);
	}
}

void AGSSnake::QueueDirection(EGSDirection NewDirection)
{
	const EGSDirection Reference = DirectionQueue.Num() > 0 ? DirectionQueue.Last() : CurrentDirection;

	// Holding a key fires Triggered every frame: ignore repeats. A snake also
	// cannot reverse into its own neck.
	if (NewDirection == Reference || GSGrid::AreOpposite(NewDirection, Reference))
	{
		return;
	}

	if (DirectionQueue.Num() >= MaxQueuedTurns)
	{
		return;
	}

	DirectionQueue.Add(NewDirection);
}

void AGSSnake::Step()
{
	if (DirectionQueue.Num() > 0)
	{
		CurrentDirection = DirectionQueue[0];
		DirectionQueue.RemoveAt(0);
	}

	const FIntPoint NextCell = Segments[0] + GSGrid::ToOffset(CurrentDirection);

	if (!Board->IsInside(NextCell) || Board->IsCellBlocked(NextCell))
	{
		Die(TEXT("hit a wall"));
		return;
	}

	// The tail tip moves away this step unless the snake is growing, so the
	// head may safely enter the cell the tail is leaving.
	const bool bTailMoves = PendingGrowth == 0;
	const int32 CellsToCheck = bTailMoves ? Segments.Num() - 1 : Segments.Num();
	for (int32 Index = 0; Index < CellsToCheck; ++Index)
	{
		if (Segments[Index] == NextCell)
		{
			Die(TEXT("bit itself"));
			return;
		}
	}

	Segments.Insert(NextCell, 0);
	if (bTailMoves)
	{
		Segments.RemoveAt(Segments.Num() - 1);
	}
	else
	{
		--PendingGrowth;
	}

	UpdateVisuals();
}

void AGSSnake::Die(const TCHAR* Reason)
{
	bMoving = false;
	bAlive = false;
	DirectionQueue.Reset();
	UE_LOG(LogGridSnake, Warning, TEXT("Snake %s at cell %s (length %d)"), Reason, *GetHeadCell().ToString(), Segments.Num());
	OnDied.Broadcast(this);
}

void AGSSnake::PlaceOnBoard()
{
	FIntPoint HeadCell = Board->WorldToCell(GetActorLocation());
	if (!Board->IsInside(HeadCell))
	{
		UE_LOG(LogGridSnake, Warning, TEXT("%s spawned outside the board at %s; using cell (1, 1)"),
			*GetName(), *GetActorLocation().ToString());
		HeadCell = FIntPoint(1, 1);
	}

	// The body trails behind the head, opposite to the starting direction.
	Segments.Reset();
	const FIntPoint Behind = GSGrid::ToOffset(CurrentDirection) * -1;
	for (int32 Index = 0; Index < InitialLength; ++Index)
	{
		Segments.Add(HeadCell + Behind * Index);
	}

	// The Player Start's rotation would otherwise tilt the sprites out of the X-Z plane.
	SetActorRotation(FRotator::ZeroRotator);
	UpdateVisuals();
	UE_LOG(LogGridSnake, Log, TEXT("Snake placed at cell %s facing %s, length %d"),
		*HeadCell.ToString(), *UEnum::GetValueAsString(CurrentDirection), Segments.Num());
}

void AGSSnake::UpdateVisuals()
{
	// Teleport: the snake jumps from cell to cell, it does not slide.
	SetActorLocation(Board->CellToWorld(Segments[0]));
	Head->SetRelativeRotation(GSGrid::ToSpriteRotation(CurrentDirection));

	const int32 NumBodySegments = Segments.Num() - 1;
	if (Body->GetInstanceCount() != NumBodySegments)
	{
		// The length changed: rebuild every instance. Cheap at snake sizes.
		Body->ClearInstances();
		for (int32 Index = 1; Index < Segments.Num(); ++Index)
		{
			const FTransform Transform(Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f));
			Body->AddInstance(Transform, BodySprite, /*bWorldSpace=*/true, FLinearColor::White);
		}
		return;
	}

	for (int32 Index = 1; Index < Segments.Num(); ++Index)
	{
		const FTransform Transform(Board->CellToWorld(Segments[Index]) + FVector(0.0f, BodyDepth, 0.0f));
		const bool bLast = Index == Segments.Num() - 1;
		Body->UpdateInstanceTransform(Index - 1, Transform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/bLast, /*bTeleport=*/true);
	}
}
