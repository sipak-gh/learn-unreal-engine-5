// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPhaseMachine.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPhase, Log, All);

void AExPhaseMachine::BeginPlay()
{
	Super::BeginPlay();

	SetPhase(EExPhase::Playing);
	SetPhase(EExPhase::Won);
	SetPhase(EExPhase::Playing); // illegal: a won game cannot resume
	SetPhase(EExPhase::Ready);   // legal: restart
}

bool AExPhaseMachine::IsLegal(EExPhase From, EExPhase To)
{
	switch (From)
	{
	case EExPhase::Ready:   return To == EExPhase::Playing;
	case EExPhase::Playing: return To == EExPhase::Won || To == EExPhase::Lost;
	case EExPhase::Won:
	case EExPhase::Lost:    return To == EExPhase::Ready;
	}
	return false;
}

bool AExPhaseMachine::SetPhase(EExPhase NewPhase)
{
	if (NewPhase == Phase)
	{
		return true;
	}

	// ensureMsgf logs the message and a call stack the first time it fails, then
	// returns the condition, so the game keeps running and we can bail out.
	if (!ensureMsgf(IsLegal(Phase, NewPhase), TEXT("Illegal phase change %s -> %s"),
		*UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase)))
	{
		return false;
	}

	UE_LOG(LogExPhase, Log, TEXT("%s -> %s"), *UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase));
	Phase = NewPhase;
	return true;
}
