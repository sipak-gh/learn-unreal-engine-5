// Learn Unreal Engine 5 - chapter 8 example.

#include "ExInputPawn.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogExInput, Log, All);

void AExInputPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Context)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, /*Priority=*/0);
		UE_LOG(LogExInput, Log, TEXT("Added %s for %s"), *Context->GetName(), *PC->GetName());
	}
}

void AExInputPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!JumpAction)
	{
		UE_LOG(LogExInput, Warning, TEXT("%s: set Jump Action in the Details panel"), *GetName());
		return;
	}

	// One handler for every event; the FInputActionInstance tells us which one fired.
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	for (const ETriggerEvent Event : { ETriggerEvent::Started, ETriggerEvent::Ongoing, ETriggerEvent::Triggered,
	                                   ETriggerEvent::Completed, ETriggerEvent::Canceled })
	{
		Input->BindAction(JumpAction, Event, this, &AExInputPawn::OnJumpEvent);
	}
}

void AExInputPawn::OnJumpEvent(const FInputActionInstance& Instance)
{
	UE_LOG(LogExInput, Log, TEXT("%s value=%s elapsed=%.2fs"),
		*UEnum::GetValueAsString(Instance.GetTriggerEvent()),
		*Instance.GetValue().ToString(),
		Instance.GetElapsedTime());
}
