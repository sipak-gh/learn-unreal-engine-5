// GridSnake - course project for "Learn Unreal Engine 5".

#include "Core/GSGameInstance.h"
#include "GridSnake.h"
#include "Core/GSSettings.h"
#include "UObject/UnrealType.h"

void UGSGameInstance::Init()
{
	Super::Init();

	const UGSSettings* Settings = UGSSettings::Get();
	UE_LOG(LogGridSnake, Log, TEXT("GSGameInstance initialised: %d gems per level, step %.2fs"),
		Settings->GemsPerLevel, Settings->DefaultStepInterval);
}

void UGSGameInstance::Shutdown()
{
	UE_LOG(LogGridSnake, Log, TEXT("GSGameInstance shutting down"));

	Super::Shutdown();
}

void UGSGameInstance::GSDumpClass(const FString& ClassName)
{
	const UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::NativeFirst);
	if (!Class)
	{
		UE_LOG(LogGridSnake, Warning, TEXT("No loaded class named '%s' (omit the A/U prefix)"), *ClassName);
		return;
	}

	// The class default object (CDO) holds every property's default value.
	const UObject* ClassDefaultObject = Class->GetDefaultObject();
	UE_LOG(LogGridSnake, Log, TEXT("Class %s (parent %s)"), *Class->GetName(), *GetNameSafe(Class->GetSuperClass()));

	for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Property = *It;

		FString DefaultValue;
		Property->ExportTextItem_Direct(DefaultValue, Property->ContainerPtrToValuePtr<void>(ClassDefaultObject), nullptr, nullptr, PPF_None);

		UE_LOG(LogGridSnake, Log, TEXT("  property %s %s = %s%s%s"),
			*Property->GetCPPType(), *Property->GetName(), *DefaultValue,
			Property->HasAnyPropertyFlags(CPF_Edit) ? TEXT(" [editable]") : TEXT(""),
			Property->HasAnyPropertyFlags(CPF_Config) ? TEXT(" [config]") : TEXT(""));
	}

	for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const UFunction* Function = *It;
		UE_LOG(LogGridSnake, Log, TEXT("  function %s%s%s"), *Function->GetName(),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) ? TEXT(" [BlueprintCallable]") : TEXT(""),
			Function->HasAnyFunctionFlags(FUNC_Exec) ? TEXT(" [Exec]") : TEXT(""));
	}
}
