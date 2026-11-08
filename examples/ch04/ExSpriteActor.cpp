// Example for chapter 4 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine, Paper2D.

#include "ExSpriteActor.h"
#include "Engine/CollisionProfile.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogExSprite, Log, All);

AExSpriteActor::AExSpriteActor()
{
	SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
	SpriteComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetRootComponent(SpriteComponent);
}

void AExSpriteActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Runs in the editor whenever the actor is placed, moved or edited.
	SpriteComponent->SetSprite(Sprite);
	SpriteComponent->SetSpriteColor(Tint);
}

void AExSpriteActor::BeginPlay()
{
	Super::BeginPlay();

	if (!Sprite)
	{
		UE_LOG(LogExSprite, Warning, TEXT("%s has no sprite; pick one in the Details panel"), *GetName());
		return;
	}

	// The size of the sprite's source region in the texture, in pixels.
	const FVector2D Size = Sprite->GetSourceSize();
	UE_LOG(LogExSprite, Log, TEXT("%s shows %s: %.0f x %.0f pixels, tint %s"),
		*GetName(), *Sprite->GetName(), Size.X, Size.Y, *Tint.ToString());
}
