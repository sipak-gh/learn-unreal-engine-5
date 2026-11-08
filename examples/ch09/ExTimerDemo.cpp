// Chapter 9 example: looping, one-shot and next-tick timers.

#include "ExTimerDemo.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTimer, Log, All);

AExTimerDemo::AExTimerDemo()
{
	// Timers are driven by the world's timer manager; this actor needs no Tick.
	PrimaryActorTick.bCanEverTick = false;
}

void AExTimerDemo::BeginPlay()
{
	Super::BeginPlay();

	FTimerManager& Timers = GetWorldTimerManager();

	// 1. Once, on the next frame. Handy for "after everyone's BeginPlay".
	Timers.SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_LOG(LogExTimer, Log, TEXT("next tick (world time %.3f)"), GetWorld()->GetTimeSeconds());
	}));

	// 2. A looping timer that calls a member function.
	Timers.SetTimer(LoopTimer, this, &AExTimerDemo::OnLoop, LoopRate, /*bLoop=*/true);

	// 3. A one-shot timer bound to a lambda. A weak lambda is skipped if this
	//    actor has been destroyed by the time the timer fires.
	Timers.SetTimer(OneShotTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_LOG(LogExTimer, Log, TEXT("one-shot fired after %d loops, clearing loop"), LoopCount);
		GetWorldTimerManager().ClearTimer(LoopTimer);
	}), OneShotDelay, /*bLoop=*/false);

	UE_LOG(LogExTimer, Log, TEXT("timers set: loop every %.2fs, one-shot in %.2fs"), LoopRate, OneShotDelay);
}

void AExTimerDemo::OnLoop()
{
	++LoopCount;
	UE_LOG(LogExTimer, Log, TEXT("loop %d (remaining on one-shot: %.2f)"),
		LoopCount, GetWorldTimerManager().GetTimerRemaining(OneShotTimer));
}

void AExTimerDemo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stop everything this actor started, even timers whose handles we lost.
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}
