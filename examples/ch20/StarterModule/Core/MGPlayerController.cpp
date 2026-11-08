// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "Core/MGPlayerController.h"
#include "MyGame.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"

void AMGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// Mapping contexts belong to the local player, so they survive pawn changes.
	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			InputSubsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
		}
		else
		{
			UE_LOG(LogMyGame, Warning, TEXT("%s has no DefaultMappingContext - set it in the Blueprint child"), *GetName());
		}
	}

	if (FixedViewTargetClass)
	{
		if (AActor* ViewTarget = UGameplayStatics::GetActorOfClass(this, FixedViewTargetClass))
		{
			// Stop possession from switching the view back to the pawn (after a respawn, say).
			bAutoManageActiveCameraTarget = false;
			SetViewTarget(ViewTarget);
			UE_LOG(LogMyGame, Log, TEXT("View target: %s"), *ViewTarget->GetName());
		}
	}
}
