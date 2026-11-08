// Learn Unreal Engine 5 - chapter 8 example.

#include "ExChordAction.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

DEFINE_LOG_CATEGORY_STATIC(LogExChord, Log, All);

void AExChordAction::CreateInputObjects()
{
	if (Context)
	{
		return;
	}

	// New input actions are Boolean (Digital) by default: right for buttons.
	ShiftAction = NewObject<UInputAction>(this, TEXT("IA_Shift"));
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	DashAction = NewObject<UInputAction>(this, TEXT("IA_Dash"));

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Chord"));

	// The chord action first, so it is evaluated before the action that depends on it.
	Context->MapKey(ShiftAction, EKeys::LeftShift);
	Context->MapKey(JumpAction, EKeys::SpaceBar);

	// Space -> Dash, but only while ShiftAction is triggered.
	UInputTriggerChordAction* Chord = NewObject<UInputTriggerChordAction>(Context);
	Chord->ChordAction = ShiftAction;
	Context->MapKey(DashAction, EKeys::SpaceBar).Triggers.Add(Chord);
}

void AExChordAction::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	CreateInputObjects();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(Context, /*Priority=*/0);
	}
}

void AExChordAction::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInputObjects();

	// Started fires once per press, unlike Triggered, which repeats while held.
	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AExChordAction::OnJump);
	Input->BindAction(DashAction, ETriggerEvent::Started, this, &AExChordAction::OnDash);
}

void AExChordAction::OnJump()
{
	UE_LOG(LogExChord, Log, TEXT("jump"));
}

void AExChordAction::OnDash()
{
	UE_LOG(LogExChord, Log, TEXT("dash"));
}
