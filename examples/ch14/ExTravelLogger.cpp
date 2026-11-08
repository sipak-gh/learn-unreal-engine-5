// Example for chapter 14 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExTravelLogger.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTravel, Log, All);

void UExTravelLogger::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Global engine events: they fire for every map load, whoever started it.
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UExTravelLogger::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UExTravelLogger::HandlePostLoadMap);

	UE_LOG(LogExTravel, Log, TEXT("Travel logger created by %s"), *GetNameSafe(GetGameInstance()));
}

void UExTravelLogger::Deinitialize()
{
	// The delegates are static and outlive us: always unbind what you bound.
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	UE_LOG(LogExTravel, Log, TEXT("Travel logger going away after %d map load(s)"), LoadCount);
	Super::Deinitialize();
}

void UExTravelLogger::HandlePreLoadMap(const FString& MapName)
{
	++LoadCount;
	UE_LOG(LogExTravel, Log, TEXT("PreLoadMap %s (load #%d)"), *MapName, LoadCount);
}

void UExTravelLogger::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(LogExTravel, Log, TEXT("PostLoadMap %s"), *GetNameSafe(LoadedWorld));
}
