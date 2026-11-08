// StarterModule - example for chapter 20 of "Learn Unreal Engine 5".

#include "Player/MGPawn2D.h"
#include "MyGame.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "InputActionValue.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

AMGPawn2D::AMGPawn2D()
{
	PrimaryActorTick.bCanEverTick = true;

	// The root is what moves and what collides. 16x16 units = one 16x16 pixel sprite
	// at 1 pixel per unit; deep in Y so it always meets walls, whatever their depth.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(8.0f, 16.0f, 8.0f));
	Box->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Box);

	Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
	Flipbook->SetupAttachment(Box);
	Flipbook->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Paper2D draws in the X-Z plane. Yaw -90 looks down -Y: +X is right, +Z is up.
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Box);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Camera->SetOrthoWidth(512.0f);
	Camera->SetRelativeLocation(FVector(0.0f, 500.0f, 0.0f));
	Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	// Simple acceleration-based movement that sweeps the root against walls.
	// The plane constraint stops it ever drifting in Y (towards or away from the camera).
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->MaxSpeed = 200.0f;
	Movement->SetPlaneConstraintEnabled(true);
	Movement->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
}

void AMGPawn2D::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMGPawn2D::OnMove);
	}
	else
	{
		UE_LOG(LogMyGame, Warning, TEXT("%s has no MoveAction - set it in the Blueprint child"), *GetName());
	}
}

void AMGPawn2D::OnMove(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	AddMovementInput(FVector::ForwardVector, Input.X); // +X: screen right
	AddMovementInput(FVector::UpVector, Input.Y);      // +Z: screen up
}

void AMGPawn2D::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pick the animation from the velocity instead of from the input, so it also looks
	// right when something else pushes the pawn.
	const FVector Velocity = GetVelocity();
	UPaperFlipbook* Wanted = Velocity.SizeSquared() > 1.0f ? RunFlipbook : IdleFlipbook;
	if (Wanted && Flipbook->GetFlipbook() != Wanted)
	{
		Flipbook->SetFlipbook(Wanted);
	}

	// Face the way we move: mirror the sprite with a negative X scale.
	if (FMath::Abs(Velocity.X) > 1.0f)
	{
		Flipbook->SetRelativeScale3D(FVector(Velocity.X < 0.0f ? -1.0f : 1.0f, 1.0f, 1.0f));
	}
}
