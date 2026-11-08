// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExDebugDraw.h"
#include "DrawDebugHelpers.h"

AExDebugDraw::AExDebugDraw()
{
	PrimaryActorTick.bCanEverTick = true;

	// An actor needs a root component to have a location.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AExDebugDraw::BeginPlay()
{
	Super::BeginPlay();

#if ENABLE_DRAW_DEBUG
	UE_LOG(LogTemp, Display, TEXT("%s: debug drawing is compiled in"), *GetName());

	// Persistent lines stay until FlushPersistentDebugLines (or the end of play).
	// Here: a cross marking where the actor started.
	const FVector Start = GetActorLocation();
	DrawDebugLine(GetWorld(), Start - FVector(8.0f, 0.0f, 0.0f), Start + FVector(8.0f, 0.0f, 0.0f), FColor::Magenta, /*bPersistentLines=*/true);
	DrawDebugLine(GetWorld(), Start - FVector(0.0f, 0.0f, 8.0f), Start + FVector(0.0f, 0.0f, 8.0f), FColor::Magenta, /*bPersistentLines=*/true);
#else
	UE_LOG(LogTemp, Display, TEXT("%s: debug drawing is compiled out (Shipping)"), *GetName());
#endif
}

void AExDebugDraw::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RunningTime += DeltaSeconds;

	// Everything inside #if is removed from builds without debug drawing, including
	// the maths that only exists to feed the shapes.
#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	const FVector Center = GetActorLocation();

	// bPersistentLines false + LifeTime -1: drawn for this frame only, so we redraw
	// every Tick. That is how you draw something that moves.
	DrawDebugBox(World, Center, FVector(BoxHalfSize, 1.0f, BoxHalfSize), FColor::Green,
		/*bPersistentLines=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, /*Thickness=*/1.0f);

	// A line turning in the X-Z plane (the plane Paper2D sprites live in).
	const float Angle = RunningTime * TurnsPerSecond * UE_TWO_PI;
	const FVector Tip = Center + FVector(FMath::Cos(Angle), 0.0f, FMath::Sin(Angle)) * ArmLength;
	DrawDebugLine(World, Center, Tip, FColor::Yellow, false, -1.0f, 0, 2.0f);
	DrawDebugSphere(World, Tip, 6.0f, /*Segments=*/12, FColor::Red, false, -1.0f, 0, 1.0f);

	// Text goes through the player's HUD. With an actor given, the location is relative
	// to it, so the label follows the actor. Duration 0: this frame only.
	DrawDebugString(World, FVector(0.0f, 0.0f, BoxHalfSize + 12.0f),
		FString::Printf(TEXT("%s  t=%.1fs"), *GetName(), RunningTime),
		this, FColor::White, /*Duration=*/0.0f, /*bDrawShadow=*/true);
#endif
}
