// Example for chapter 19 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExStatsAndTrace.h"
#include "HAL/PlatformTime.h"
#include "Logging/StructuredLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"

// A log category private to this file.
DEFINE_LOG_CATEGORY_STATIC(LogExStats, Log, All);

// A stat group is what "stat <Name>" shows: here "stat Example".
DECLARE_STATS_GROUP(TEXT("Example"), STATGROUP_Example, STATCAT_Advanced);

// One cycle counter in that group. The text is the row's label on screen.
DECLARE_CYCLE_STAT(TEXT("Sum Values"), STAT_ExSumValues, STATGROUP_Example);

AExStatsAndTrace::AExStatsAndTrace()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AExStatsAndTrace::BeginPlay()
{
	Super::BeginPlay();

	// FPlatformTime::Seconds() is a high-resolution clock: fine for a one-off timing.
	const double StartSeconds = FPlatformTime::Seconds();
	LastSum = SumValues(ValuesPerFrame);
	const double Ms = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

	// Structured logging: the {Names} are filled in order by the values that follow.
	UE_LOGFMT(LogExStats, Log, "Summed {Count} values in {Ms} ms", ValuesPerFrame, Ms);
	UE_LOGFMT(LogExStats, Verbose, "Sum was {Sum}", LastSum);
}

void AExStatsAndTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LastSum = SumValues(ValuesPerFrame);
}

uint64 AExStatsAndTrace::SumValues(int32 Count)
{
	// Both scopes end at the closing brace, so they time the whole function.
	// The cycle counter feeds "stat Example"; the trace scope is a named block in
	// Unreal Insights' timeline. Both cost almost nothing when nobody is looking,
	// and both are compiled out of Shipping builds by default.
	SCOPE_CYCLE_COUNTER(STAT_ExSumValues);
	TRACE_CPUPROFILER_EVENT_SCOPE(AExStatsAndTrace::SumValues);

	uint64 Sum = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// A cheap hash, so the compiler cannot replace the loop with a formula.
		Sum += (uint64(Index) * 2654435761ull) % 1000ull;
	}
	return Sum;
}
