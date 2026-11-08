// Learn Unreal Engine 5 - chapter 8 example.

#include "ExRuntimeInput.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

DEFINE_LOG_CATEGORY_STATIC(LogExRuntime, Log, All);

void AExRuntimeInput::CreateInputObjects()
{
	if (MoveAction)
	{
		return;
	}

	// The same settings you would make in the Input Action editor.
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_RuntimeMove"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Runtime"));

	// Each helper adds one modifier to a mapping, like the + in the context editor.
	auto AddSwizzle = [this](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
		Swizzle->Order = EInputAxisSwizzle::YXZ; // X goes to Y: a key now pushes "up"
		Mapping.Modifiers.Add(Swizzle);
	};
	auto AddNegate = [this](FEnhancedActionKeyMapping& Mapping)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
	};

	// MapKey returns a reference into the context's array. Use it before the
	// next MapKey call, which may reallocate the array.
	Context->MapKey(MoveAction, EKeys::D);                // (1, 0)
	AddNegate(Context->MapKey(MoveAction, EKeys::A));     // (-1, 0)
	AddSwizzle(Context->MapKey(MoveAction, EKeys::W));    // (0, 1)
	FEnhancedActionKeyMapping& Down = Context->MapKey(MoveAction, EKeys::S);
	AddSwizzle(Down);                                     // (0, 1)
	AddNegate(Down);                                      // (0, -1)
}

void AExRuntimeInput::NotifyControllerChanged()
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

void AExRuntimeInput::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInputObjects();

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExRuntimeInput::OnMove);
	// Completed fires once when the last key is released.
	Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AExRuntimeInput::OnMoveReleased);
}

void AExRuntimeInput::OnMove(const FInputActionValue& Value)
{
	// Triggered fires every frame while a key is held; log only changes.
	const FVector2D Move = Value.Get<FVector2D>();
	if (!Move.Equals(LastValue))
	{
		LastValue = Move;
		UE_LOG(LogExRuntime, Log, TEXT("Move %s"), *Move.ToString());
	}
}

void AExRuntimeInput::OnMoveReleased()
{
	LastValue = FVector2D::ZeroVector;
	UE_LOG(LogExRuntime, Log, TEXT("Move released"));
}
