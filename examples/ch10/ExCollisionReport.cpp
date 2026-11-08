// Chapter 10 example: a component that reports its owner's collision settings.

#include "ExCollisionReport.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogExCollision, Log, All);

namespace
{
	// ECollisionEnabled is an old-style namespaced enum (ECollisionEnabled::Type).
	// A switch gives short names; newer "probe" values share the default branch.
	const TCHAR* CollisionEnabledName(ECollisionEnabled::Type Enabled)
	{
		switch (Enabled)
		{
		case ECollisionEnabled::NoCollision:     return TEXT("NoCollision");
		case ECollisionEnabled::QueryOnly:       return TEXT("QueryOnly");
		case ECollisionEnabled::PhysicsOnly:     return TEXT("PhysicsOnly");
		case ECollisionEnabled::QueryAndPhysics: return TEXT("QueryAndPhysics");
		default:                                 return TEXT("Other (probe)");
		}
	}
}

UExCollisionReport::UExCollisionReport()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UExCollisionReport::BeginPlay()
{
	Super::BeginPlay();

	TArray<UPrimitiveComponent*> Primitives;
	GetOwner()->GetComponents(Primitives);
	UE_LOG(LogExCollision, Log, TEXT("%s has %d primitive component(s)"), *GetOwner()->GetName(), Primitives.Num());

	const FString ChannelName = UEnum::GetValueAsString(ChannelToReport.GetValue());
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		UE_LOG(LogExCollision, Log, TEXT("  %s: profile %s, %s, object type %s, overlap events %s, %s -> %s"),
			*Primitive->GetName(),
			*Primitive->GetCollisionProfileName().ToString(),
			CollisionEnabledName(Primitive->GetCollisionEnabled()),
			*UEnum::GetValueAsString(Primitive->GetCollisionObjectType()),
			Primitive->GetGenerateOverlapEvents() ? TEXT("on") : TEXT("off"),
			*ChannelName,
			*UEnum::GetValueAsString(Primitive->GetCollisionResponseToChannel(ChannelToReport)));
	}
}
