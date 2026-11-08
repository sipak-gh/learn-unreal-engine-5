// Chapter 10 example: a trigger box that logs what enters and leaves it.

#include "ExTriggerBox.h"
#include "Components/BoxComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExTrigger, Log, All);

AExTriggerBox::AExTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->InitBoxExtent(FVector(32.0f, 32.0f, 32.0f));
	// A WorldDynamic object that overlaps every channel and blocks nothing.
	Box->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// Not part of the profile: both components of a pair need it for events.
	Box->SetGenerateOverlapEvents(true);
	// Draw the wireframe while playing, not only in the editor.
	Box->SetHiddenInGame(false);
	SetRootComponent(Box);
}

void AExTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &AExTriggerBox::HandleBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &AExTriggerBox::HandleEndOverlap);

	// Anything that was already inside before the bindings existed raised no event.
	TArray<AActor*> AlreadyInside;
	Box->GetOverlappingActors(AlreadyInside);
	UE_LOG(LogExTrigger, Log, TEXT("%s ready, %d actor(s) already inside"), *GetName(), AlreadyInside.Num());
}

void AExTriggerBox::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogExTrigger, Log, TEXT("%s entered (component %s, from sweep: %s)"),
		*GetNameSafe(OtherActor), *GetNameSafe(OtherComp), bFromSweep ? TEXT("yes") : TEXT("no"));
}

void AExTriggerBox::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogExTrigger, Log, TEXT("%s left (component %s)"), *GetNameSafe(OtherActor), *GetNameSafe(OtherComp));
}
