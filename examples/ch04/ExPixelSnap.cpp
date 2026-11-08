// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExPixelSnap.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPixel, Log, All);

FVector UExPixelSnapLibrary::SnapToPixelGrid(FVector Location, float Unit)
{
	if (Unit <= 0.0f)
	{
		return Location;
	}

	// FVector holds doubles in UE5, so snap in double precision.
	const double Grid = Unit;
	return FVector(FMath::GridSnap(Location.X, Grid), Location.Y, FMath::GridSnap(Location.Z, Grid));
}

void UExPixelSnapLibrary::LogSnapExamples()
{
	const FVector Samples[] = { FVector(23.4, 5.0, -7.6), FVector(25.0, 0.0, -25.0) };
	const float Units[] = { 1.0f, 16.0f };

	for (const FVector& Sample : Samples)
	{
		for (const float Unit : Units)
		{
			UE_LOG(LogExPixel, Log, TEXT("Snap(%s, %.0f) = %s"),
				*Sample.ToString(), Unit, *SnapToPixelGrid(Sample, Unit).ToString());
		}
	}
}
