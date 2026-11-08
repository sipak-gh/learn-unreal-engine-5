// Example for chapter 6 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExOrthoCamera.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AExOrthoCamera::AExOrthoCamera()
{
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
}

void AExOrthoCamera::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Apply the property to the component so the editor preview matches.
	Camera->SetOrthoWidth(OrthoWidth);
}

void AExOrthoCamera::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no player controller to take over"), *GetName());
		return;
	}

	// The view target is the actor the player's camera manager looks through.
	// An actor with a camera component supplies that component's view.
	PlayerController->SetViewTargetWithBlend(this, BlendTime);
	UE_LOG(LogTemp, Log, TEXT("%s is now the view target (ortho width %.0f)"), *GetName(), OrthoWidth);
}

void AExOrthoCamera::ZoomTo(float NewWidth)
{
	OrthoWidth = FMath::Max(1.0f, NewWidth);
	Camera->SetOrthoWidth(OrthoWidth);
	UE_LOG(LogTemp, Log, TEXT("%s ortho width %.0f"), *GetName(), OrthoWidth);
}
