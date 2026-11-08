// GridSnake - course project for "Learn Unreal Engine 5".

#include "Snake/GSSnake.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Core/GSSettings.h"
#include "Curves/CurveFloat.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "Gameplay/GSBoard.h"
#include "Gameplay/GSSnakeInteractable.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperGroupedSpriteComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"
#include "VisualLogger/VisualLogger.h"

// Shows up under "stat GridSnake" (call count and time per frame).
DECLARE_CYCLE_STAT(TEXT("Snake Step"), STAT_GSSnakeStep, STATGROUP_GridSnake);

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

	BaseStepInterval = UGSSettings::Get()->DefaultStepInterval;
	CurrentDirection = Board->GetStartDirection();
	PlaceOnBoard();

	Head->OnFinishedPlaying.AddDynamic(this, &AGSSnake::HandleHeadAnimFinished);
	PlayHeadAnim(EGSHeadAnim::Idle);
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
	float Interval = GetStepInterval();
	while (bMoving && StepAccumulator >= Interval)
	{
		StepAccumulator -= Interval;
		Step();
		Interval = GetStepInterval();
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
	PlayHeadAnim(EGSHeadAnim::Move);
	UE_LOG(LogGridSnake, Log, TEXT("Snake starts moving %s, one step every %.2fs"),
		*UEnum::GetValueAsString(CurrentDirection), GetStepInterval());
}

void AGSSnake::ConfigureSpeed(float BaseInterval, UCurveFloat* Curve)
{
	BaseStepInterval = FMath::Max(BaseInterval, 0.02f);
	SpeedCurve = Curve;
}

float AGSSnake::GetStepInterval() const
{
	const float Multiplier = SpeedCurve ? SpeedCurve->GetFloatValue(float(Segments.Num())) : 1.0f;
	// Never step faster than 50 times a second, whatever the curve says.
	return FMath::Max(BaseStepInterval * Multiplier, 0.02f);
}

void AGSSnake::StopMoving()
{
	bMoving = false;
	DirectionQueue.Reset();
	if (bAlive)
	{
		PlayHeadAnim(EGSHeadAnim::Idle);
	}
}

void AGSSnake::Grow(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	PendingGrowth += Amount;
	if (bAlive)
	{
		PlayHeadAnim(EGSHeadAnim::Eat);
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
	// Stats feed "stat GridSnake"; the trace scope shows each step as a named
	// block on the timeline in Unreal Insights.
	SCOPE_CYCLE_COUNTER(STAT_GSSnakeStep);
	TRACE_CPUPROFILER_EVENT_SCOPE(AGSSnake::Step);

	// check() stops the program in Debug and Development builds if it fails:
	// for things that must never happen. Tick only steps a live snake on a board.
	check(Board && Segments.Num() > 0);

	if (DirectionQueue.Num() > 0)
	{
		CurrentDirection = DirectionQueue[0];
		DirectionQueue.RemoveAt(0);
	}

	const FIntPoint NextCell = Segments[0] + GSGrid::ToOffset(CurrentDirection);

	// Recorded by the Visual Logger (Tools > Debug > Visual Logger) when it is
	// recording; costs next to nothing otherwise.
	UE_VLOG_LOCATION(this, LogGridSnake, Verbose, Board->CellToWorld(NextCell), 4.0f, FColor::Yellow,
		TEXT("Step %s -> %s"), *UEnum::GetValueAsString(CurrentDirection), *NextCell.ToString());

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
	// Structured logging: {Name} placeholders are filled, in order, by the values
	// that follow, and are kept as named fields for tools that read the log.
	const FIntPoint Cell = GetHeadCell();
	UE_LOGFMT(LogGridSnake, Warning, "Snake {Reason} at cell ({X}, {Y}), length {Length}", FString(Reason), Cell.X, Cell.Y, Segments.Num());
	UE_VLOG(this, LogGridSnake, Warning, TEXT("Died: %s"), Reason);

	PlayHeadAnim(EGSHeadAnim::Dead);
	if (DeathSound)
	{
		UGameplayStatics::PlaySound2D(this, DeathSound);
	}

	DeathBlinksLeft = DeathBlinks * 2;
	if (DeathBlinksLeft > 0)
	{
		GetWorldTimerManager().SetTimer(DeathBlinkTimer, this, &AGSSnake::ToggleBodyVisibility, 0.1f, /*bLoop=*/true);
	}

	OnDied.Broadcast(this);
}

void AGSSnake::ToggleBodyVisibility()
{
	Body->SetVisibility(!Body->IsVisible());
	if (--DeathBlinksLeft <= 0)
	{
		Body->SetVisibility(true);
		GetWorldTimerManager().ClearTimer(DeathBlinkTimer);
	}
}

void AGSSnake::PlayHeadAnim(EGSHeadAnim NewAnim)
{
	UPaperFlipbook* Flipbook = nullptr;
	bool bLoop = true;
	switch (NewAnim)
	{
	case EGSHeadAnim::Idle: Flipbook = IdleFlipbook; break;
	case EGSHeadAnim::Move: Flipbook = MoveFlipbook; break;
	case EGSHeadAnim::Eat:  Flipbook = EatFlipbook;  bLoop = false; break;
	case EGSHeadAnim::Dead: Flipbook = DeadFlipbook; bLoop = false; break;
	}

	HeadAnim = NewAnim;
	if (!Flipbook)
	{
		return;
	}

	// The state machine is just this switch: pick a flipbook, decide whether
	// it loops, restart it. One-shot states end in HandleHeadAnimFinished().
	Head->SetFlipbook(Flipbook);
	Head->SetLooping(bLoop);
	Head->PlayFromStart();
}

void AGSSnake::HandleHeadAnimFinished()
{
	// Only non-looping flipbooks finish. Dead stays on its last frame.
	if (HeadAnim == EGSHeadAnim::Eat && bAlive)
	{
		PlayHeadAnim(bMoving ? EGSHeadAnim::Move : EGSHeadAnim::Idle);
	}
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
			Body->AddInstance(Transform, BodySprite, /*bWorldSpace=*/true, GetSegmentColor(Index));
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

FLinearColor AGSSnake::GetSegmentColor(int32 SegmentIndex) const
{
	// 0 for the first body segment, 1 for the tail tip.
	const int32 NumBodySegments = Segments.Num() - 1;
	const float Alpha = NumBodySegments > 1 ? float(SegmentIndex - 1) / float(NumBodySegments - 1) : 0.0f;
	return FLinearColor::LerpUsingHSV(BodyHeadColor, BodyTailColor, Alpha);
}
