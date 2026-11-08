// Example for chapter 12 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExFlipbookStates.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExAnim, Log, All);

AExFlipbookStates::AExFlipbookStates()
{
	PrimaryActorTick.bCanEverTick = false;

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	SetRootComponent(Sprite);
}

void AExFlipbookStates::BeginPlay()
{
	Super::BeginPlay();

	Sprite->OnFinishedPlaying.AddDynamic(this, &AExFlipbookStates::HandleFinished);
	UE_LOG(LogExAnim, Log, TEXT("%s starts in %s"), *GetName(), *UEnum::GetValueAsString(State));
	PlayFlipbookFor(State);

	GetWorldTimerManager().SetTimer(Timer, this, &AExFlipbookStates::Advance, 1.0f, /*bLoop=*/true);
}

void AExFlipbookStates::Advance()
{
	// The timer drives Idle -> Walk -> Attack. Attack is left by its own end, not by the timer.
	if (State == EExAnimState::Idle)
	{
		EnterState(EExAnimState::Walk);
	}
	else if (State == EExAnimState::Walk)
	{
		EnterState(EExAnimState::Attack);
	}
}

void AExFlipbookStates::EnterState(EExAnimState NewState)
{
	if (NewState == State)
	{
		return; // never restart the running animation by accident
	}

	UE_LOG(LogExAnim, Log, TEXT("%s: %s -> %s"), *GetName(),
		*UEnum::GetValueAsString(State), *UEnum::GetValueAsString(NewState));
	State = NewState;
	PlayFlipbookFor(State);
}

void AExFlipbookStates::PlayFlipbookFor(EExAnimState ForState)
{
	const TObjectPtr<UPaperFlipbook>* Found = Flipbooks.Find(ForState);
	if (!Found || !*Found)
	{
		UE_LOG(LogExAnim, Warning, TEXT("%s has no flipbook for %s"), *GetName(), *UEnum::GetValueAsString(ForState));
		return;
	}

	Sprite->SetFlipbook(*Found);                         // resets play time if it is a new flipbook
	Sprite->SetLooping(ForState != EExAnimState::Attack); // only non-looping flipbooks finish
	Sprite->PlayFromStart();
}

void AExFlipbookStates::HandleFinished()
{
	EnterState(EExAnimState::Idle);
}
