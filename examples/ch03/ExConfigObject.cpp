// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExConfigObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogExConfig, Log, All);

void AExConfigDemo::BeginPlay()
{
	Super::BeginPlay();

	// The class default object (CDO) was filled from the .ini files when the class loaded.
	const UExConfigObject* Defaults = GetDefault<UExConfigObject>();
	UE_LOG(LogExConfig, Log, TEXT("HighScore read from config: %d"), Defaults->HighScore);

	// A new object starts as a copy of the CDO. Change it and save its Config properties.
	UExConfigObject* Copy = NewObject<UExConfigObject>();
	Copy->HighScore = Defaults->HighScore + 1;
	Copy->SaveConfig();

	// Reload the CDO from the (now updated) config cache so the next Play sees the new value.
	GetMutableDefault<UExConfigObject>()->ReloadConfig();

	UE_LOG(LogExConfig, Log, TEXT("Saved HighScore=%d to section [%s]"),
		Copy->HighScore, *UExConfigObject::StaticClass()->GetPathName());
}
