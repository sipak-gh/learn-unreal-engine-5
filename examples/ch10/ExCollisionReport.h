// Chapter 10 example: a component that reports its owner's collision settings.
// Build.cs modules: Core, CoreUObject, Engine.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExCollisionReport.generated.h"

/** Add to any actor or Blueprint (Add > Ex Collision Report) and press Play. */
UCLASS(ClassGroup = (Debug), meta = (BlueprintSpawnableComponent))
class UExCollisionReport : public UActorComponent
{
	GENERATED_BODY()

public:
	UExCollisionReport();

protected:
	virtual void BeginPlay() override;

	/** The channel whose response is reported for every primitive component. */
	UPROPERTY(EditAnywhere, Category = "Report")
	TEnumAsByte<ECollisionChannel> ChannelToReport = ECC_Visibility;
};
