// Chapter 9 example: one grouped sprite component drawing a 10x10 grid.

#include "ExGroupedSprites.h"
#include "PaperGroupedSpriteComponent.h"
#include "PaperSprite.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExGrouped, Log, All);

AExGroupedSprites::AExGroupedSprites()
{
	PrimaryActorTick.bCanEverTick = false;

	Grid = CreateDefaultSubobject<UPaperGroupedSpriteComponent>(TEXT("Grid"));
	SetRootComponent(Grid);
}

void AExGroupedSprites::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor on every edit, so the grid is visible without playing.
	Grid->ClearInstances();
	if (!Sprite)
	{
		return;
	}

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			// Local space: X goes right, rows go down the screen (-Z).
			const FTransform Local(FVector(Column * Spacing, 0.0f, -Row * Spacing));
			Grid->AddInstance(Local, Sprite, /*bWorldSpace=*/false, FLinearColor::White);
		}
	}
}

void AExGroupedSprites::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogExGrouped, Log, TEXT("%s draws %d instances with one component"),
		*GetName(), Grid->GetInstanceCount());

	GetWorldTimerManager().SetTimer(RecolourTimer, this, &AExGroupedSprites::RecolourRandomInstance,
		RecolourInterval, /*bLoop=*/true);
}

void AExGroupedSprites::RecolourRandomInstance()
{
	const int32 Count = Grid->GetInstanceCount();
	if (Count == 0)
	{
		return;
	}

	const int32 Index = FMath::RandRange(0, Count - 1);
	const FLinearColor Color = FLinearColor::MakeRandomColor();

	// true: mark the render state dirty so the change shows this frame.
	Grid->UpdateInstanceColor(Index, Color, /*bMarkRenderStateDirty=*/true);
	UE_LOG(LogExGrouped, Log, TEXT("instance %d is now #%s"), Index, *Color.ToFColor(true).ToHex());
}
