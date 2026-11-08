// Example for chapter 5 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExLifecycleLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogExLife, Log, All);

AExLifecycleLogger::AExLifecycleLogger()
{
	// The constructor also runs for the CDO, at engine start-up, before any level exists.
	UE_LOG(LogExLife, Log, TEXT("Constructor %s (CDO: %s)"), *GetName(),
		HasAnyFlags(RF_ClassDefaultObject) ? TEXT("true") : TEXT("false"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AExLifecycleLogger::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LogStage(TEXT("OnConstruction"));
}

void AExLifecycleLogger::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	LogStage(TEXT("PostInitializeComponents"));
}

void AExLifecycleLogger::BeginPlay()
{
	Super::BeginPlay();
	LogStage(TEXT("BeginPlay"));

	if (DestroyAfterSeconds > 0.0f)
	{
		// Destroy() is called for us when the life span runs out.
		SetLifeSpan(DestroyAfterSeconds);
	}
}

void AExLifecycleLogger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogExLife, Log, TEXT("EndPlay %s reason %s"), *GetName(), *UEnum::GetValueAsString(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}

void AExLifecycleLogger::Destroyed()
{
	Super::Destroyed();
	LogStage(TEXT("Destroyed"));
}

void AExLifecycleLogger::LogStage(const TCHAR* Stage) const
{
	UE_LOG(LogExLife, Log, TEXT("%s %s"), Stage, *GetName());
}
