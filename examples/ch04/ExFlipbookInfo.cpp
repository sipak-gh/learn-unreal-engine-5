// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExFlipbookInfo.h"
#include "Engine/CollisionProfile.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExFlipbook, Log, All);

AExFlipbookInfo::AExFlipbookInfo()
{
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("FlipbookComponent"));
	FlipbookComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(FlipbookComponent);
}

void AExFlipbookInfo::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	FlipbookComponent->SetFlipbook(Flipbook);
	FlipbookComponent->SetPlayRate(PlayRate);
}

void AExFlipbookInfo::BeginPlay()
{
	Super::BeginPlay();

	if (!Flipbook)
	{
		UE_LOG(LogExFlipbook, Warning, TEXT("%s has no flipbook; pick one in the Details panel"), *GetName());
		return;
	}

	// These read the flipbook asset; the play rate does not change them.
	UE_LOG(LogExFlipbook, Log, TEXT("%s: %d frames at %.1f fps = %.2f s (play rate %.1f)"),
		*Flipbook->GetName(),
		FlipbookComponent->GetFlipbookLengthInFrames(),
		FlipbookComponent->GetFlipbookFramerate(),
		FlipbookComponent->GetFlipbookLength(),
		FlipbookComponent->GetPlayRate());
}
