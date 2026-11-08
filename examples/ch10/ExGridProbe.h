// Chapter 10 example: scene queries - a box overlap test and a line trace.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExGridProbe.generated.h"

/** Place in a level, drag the TraceEnd diamond onto something solid, press Play. */
UCLASS()
class AExGridProbe : public AActor
{
	GENERATED_BODY()

public:
	AExGridProbe();

	/** True if anything that blocks Channel overlaps a cube of HalfSize around Center. */
	UFUNCTION(BlueprintCallable, Category = "Probe")
	bool IsBlocked(FVector Center, float HalfSize, ECollisionChannel Channel) const;

protected:
	virtual void BeginPlay() override;

	/** The trace channel both queries use. Custom channels appear by their names. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Half the side of the box tested at the actor's location. */
	UPROPERTY(EditAnywhere, Category = "Probe", meta = (ClampMin = "0.1"))
	float ProbeHalfSize = 4.0f;

	/** End of the line trace, relative to the actor. MakeEditWidget adds a viewport handle. */
	UPROPERTY(EditAnywhere, Category = "Probe", meta = (MakeEditWidget))
	FVector TraceEnd = FVector(100.0f, 0.0f, 0.0f);
};
