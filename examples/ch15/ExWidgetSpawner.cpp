// Example for chapter 15 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, UMG.

#include "ExWidgetSpawner.h"
#include "ExCounterWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSpawner, Log, All);

void AExWidgetSpawner::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!WidgetClass || !PlayerController)
	{
		UE_LOG(LogExSpawner, Warning, TEXT("%s: set Widget Class (and play with a player)"), *GetName());
		return;
	}

	// The owning player decides whose screen the widget is on and who gives it input.
	Widget = CreateWidget<UExCounterWidget>(PlayerController, WidgetClass);
	Widget->AddToViewport();
	UE_LOG(LogExSpawner, Log, TEXT("Widget shown: %s"), *Widget->GetName());

	GetWorldTimerManager().SetTimer(CountTimer, this, &AExWidgetSpawner::AddOne, 1.0f, /*bLoop=*/true);
}

void AExWidgetSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CountTimer);
	if (Widget)
	{
		Widget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}

void AExWidgetSpawner::AddOne()
{
	Widget->SetCount(Widget->GetCount() + 1);
	UE_LOG(LogExSpawner, Log, TEXT("Count: %d"), Widget->GetCount());
}
