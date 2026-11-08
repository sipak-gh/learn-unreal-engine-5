// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSExit.h"
#include "GridSnake.h"
#include "Components/BoxComponent.h"
#include "Core/GSGameMode.h"
#include "Core/GSGameState.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/GSBoard.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"

AGSExit::AGSExit()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	SetRootComponent(Trigger);

	// Only ever answers Grid queries; physics and other traces ignore it.
	Blocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(Trigger);
	Blocker->InitBoxExtent(FVector(6.0f, 16.0f, 6.0f));
	Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Blocker->SetCollisionObjectType(ECC_WorldStatic);
	Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
	Blocker->SetCollisionResponseToChannel(ECC_GSGrid, ECR_Block);
	Blocker->SetGenerateOverlapEvents(false);

	Door = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Door"));
	Door->SetupAttachment(Trigger);
	Door->SetRelativeLocation(FVector(0.0f, 5.0f, 0.0f));
	Door->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AGSExit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor whenever the actor is placed, moved or edited, so the
	// level shows the right door state without pressing Play.
	UpdateVisuals();
	UpdateCollision();
}

void AGSExit::BeginPlay()
{
	Super::BeginPlay();

	if (const AGSBoard* Board = Cast<AGSBoard>(UGameplayStatics::GetActorOfClass(this, AGSBoard::StaticClass())))
	{
		Board->SnapActorToCell(this);
	}

	// Event-driven: the exit is told when to open instead of polling the gem count.
	if (AGSGameState* GameState = GetWorld()->GetGameState<AGSGameState>())
	{
		GameState->OnAllGemsCollected.AddDynamic(this, &AGSExit::HandleAllGemsCollected);
	}

	UE_LOG(LogGridSnake, Log, TEXT("%s starts %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::OnSnakeEntered_Implementation(AGSSnake* Snake)
{
	// A closed exit blocks the Grid channel, so the snake dies before entering;
	// this check is a safety net for exits opened and closed by other code.
	if (!bIsOpen)
	{
		return;
	}

	if (AGSGameMode* GameMode = GetWorld()->GetAuthGameMode<AGSGameMode>())
	{
		GameMode->OnExitReached(Snake);
	}
}

void AGSExit::HandleAllGemsCollected()
{
	SetOpen(true);
}

void AGSExit::SetOpen(bool bNewOpen)
{
	if (bIsOpen == bNewOpen)
	{
		return;
	}

	bIsOpen = bNewOpen;
	UpdateVisuals();
	UpdateCollision();
	UE_LOG(LogGridSnake, Log, TEXT("%s is now %s"), *GetName(), bIsOpen ? TEXT("open") : TEXT("closed"));
}

void AGSExit::UpdateVisuals()
{
	UPaperFlipbook* Wanted = bIsOpen ? OpenFlipbook : ClosedFlipbook;
	if (Wanted)
	{
		Door->SetFlipbook(Wanted);
		Door->PlayFromStart();
	}
}

void AGSExit::UpdateCollision()
{
	Blocker->SetCollisionResponseToChannel(ECC_GSGrid, bIsOpen ? ECR_Ignore : ECR_Block);
}
