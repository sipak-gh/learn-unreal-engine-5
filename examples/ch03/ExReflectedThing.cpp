// Example for chapter 3 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExReflectedThing.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogExReflect, Log, All);

UExReflectedThing::UExReflectedThing()
{
	// Constructors set defaults. This one also runs for the class default object.
	Tags = { TEXT("Shiny"), TEXT("Small") };
}

FString UExReflectedThing::Describe() const
{
	return FString::Printf(TEXT("%s x%d"), *Label, Count);
}

void AExReflectionDemo::BeginPlay()
{
	Super::BeginPlay();

	// The Outer (this actor) is the object's owner for naming and lifetime grouping.
	Thing = NewObject<UExReflectedThing>(this);
	const UClass* Class = Thing->GetClass();
	UE_LOG(LogExReflect, Log, TEXT("%s is a %s, outer %s"),
		*Thing->GetName(), *Class->GetName(), *GetNameSafe(Thing->GetOuter()));

	// Walk the reflected properties. Hidden is not one of them.
	for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;

		// GetCPPType returns "TArray" and puts "<FName>" into the extended text.
		FString ExtendedType;
		const FString Type = Property->GetCPPType(&ExtendedType);

		FString Value;
		Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Thing.Get()),
			nullptr, nullptr, PPF_None);

		UE_LOG(LogExReflect, Log, TEXT("Property %s (%s%s) = %s"),
			*Property->GetName(), *Type, *ExtendedType, *Value);
	}

	for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UE_LOG(LogExReflect, Log, TEXT("Function %s"), *It->GetName());
	}

	// Write a property by name, the way the Details panel and the config system do.
	const FName CountName = GET_MEMBER_NAME_CHECKED(UExReflectedThing, Count);
	if (const FIntProperty* CountProperty = CastField<FIntProperty>(Class->FindPropertyByName(CountName)))
	{
		CountProperty->SetPropertyValue_InContainer(Thing.Get(), 10);
	}
	UE_LOG(LogExReflect, Log, TEXT("After writing Count by name: %s"), *Thing->Describe());
}
