// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPersistentCounter.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Lives in the module: it survives level changes and, in the editor, even Stop and Play. */
	int32 GStaticCount = 0;

	const TCHAR* ReasonName(EEndPlayReason::Type Reason)
	{
		switch (Reason)
		{
		case EEndPlayReason::Destroyed:        return TEXT("Destroyed");
		case EEndPlayReason::LevelTransition:  return TEXT("LevelTransition");
		case EEndPlayReason::EndPlayInEditor:  return TEXT("EndPlayInEditor");
		case EEndPlayReason::RemovedFromWorld: return TEXT("RemovedFromWorld");
		case EEndPlayReason::Quit:             return TEXT("Quit");
		default:                               return TEXT("?");
		}
	}
}

void AExCounterProbe::BeginPlay()
{
	Super::BeginPlay();

	UExPersistentCounter* Counter = GetGameInstance()->GetSubsystem<UExPersistentCounter>();
	const int32 SubsystemCount = Counter->Increment();
	++ActorCount;
	++GStaticCount;

	UE_LOG(LogTemp, Log, TEXT("Probe on %s: subsystem %d, actor %d, static %d"),
		*UGameplayStatics::GetCurrentLevelName(this), SubsystemCount, ActorCount, GStaticCount);
}

void AExCounterProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("Probe on %s EndPlay: %s"),
		*UGameplayStatics::GetCurrentLevelName(this), ReasonName(EndPlayReason));

	Super::EndPlay(EndPlayReason);
}
