// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExGcDemo.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExGc, Log, All);

void UExGcPayload::BeginDestroy()
{
	UE_LOG(LogExGc, Log, TEXT("%s: BeginDestroy"), *GetName());
	Super::BeginDestroy();
}

void AExGcDemo::BeginPlay()
{
	Super::BeginPlay();

	// Explicit names make the log easy to read. The Outer (this) does not keep them alive.
	Kept = NewObject<UExGcPayload>(this, TEXT("KeptPayload"));
	Weak = NewObject<UExGcPayload>(this, TEXT("WeakPayload"));
	Raw = NewObject<UExGcPayload>(this, TEXT("RawPayload"));
	UE_LOG(LogExGc, Log, TEXT("Created three payloads; requesting a full garbage collection"));

	// The collection runs on the next engine tick, not inside this call.
	GEngine->ForceGarbageCollection(true);

	GetWorldTimerManager().SetTimer(CheckTimer, this, &AExGcDemo::CheckPointers, 0.5f, false);
}

void AExGcDemo::CheckPointers()
{
	UE_LOG(LogExGc, Log, TEXT("Kept valid: %s, Weak valid: %s"),
		IsValid(Kept) ? TEXT("true") : TEXT("false"),
		Weak.IsValid() ? TEXT("true") : TEXT("false"));

	// Comparing the address with nullptr is all we may do: the memory behind it is gone.
	UE_LOG(LogExGc, Log, TEXT("Raw is still non-null: %s (it dangles)"),
		Raw != nullptr ? TEXT("true") : TEXT("false"));
	Raw = nullptr;
}
