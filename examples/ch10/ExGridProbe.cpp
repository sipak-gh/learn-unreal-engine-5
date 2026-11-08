// Chapter 10 example: scene queries - a box overlap test and a line trace.

#include "ExGridProbe.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogExProbe, Log, All);

AExGridProbe::AExGridProbe()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool AExGridProbe::IsBlocked(FVector Center, float HalfSize, ECollisionChannel Channel) const
{
	const FCollisionShape Probe = FCollisionShape::MakeBox(FVector(HalfSize));
	// The stat name shows up in profiling tools; the actor ignores itself.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExGridProbe), /*bTraceComplex=*/false, this);
	return GetWorld()->OverlapBlockingTestByChannel(Center, FQuat::Identity, Channel, Probe, Params);
}

void AExGridProbe::BeginPlay()
{
	Super::BeginPlay();

	const FVector Start = GetActorLocation();
	const FVector End = GetActorTransform().TransformPosition(TraceEnd);
	const FString ChannelName = UEnum::GetValueAsString(TraceChannel.GetValue());

	UE_LOG(LogExProbe, Log, TEXT("Box at %s is %s on %s"), *Start.ToString(),
		IsBlocked(Start, ProbeHalfSize, TraceChannel) ? TEXT("blocked") : TEXT("free"), *ChannelName);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExGridProbeTrace), /*bTraceComplex=*/false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
	{
		UE_LOG(LogExProbe, Log, TEXT("Hit %s (component %s) at distance %.1f"),
			*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), Hit.Distance);
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, 10.0f);
	}
	else
	{
		UE_LOG(LogExProbe, Log, TEXT("Nothing blocks %s between %s and %s"),
			*ChannelName, *Start.ToString(), *End.ToString());
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 10.0f);
	}
}
