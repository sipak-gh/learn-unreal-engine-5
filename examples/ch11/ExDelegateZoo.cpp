// Example for chapter 11 of "Learn Unreal Engine 5". Modules: Core, CoreUObject, Engine.

#include "ExDelegateZoo.h"

DEFINE_LOG_CATEGORY_STATIC(LogExDelegates, Log, All);

void AExDelegateZoo::BeginPlay()
{
	Super::BeginPlay();

	// 1. Single delegate. Execute() on an unbound delegate asserts, so check first.
	Square.BindLambda([](int32 Value) { return Value * Value; });
	if (Square.IsBound())
	{
		UE_LOG(LogExDelegates, Log, TEXT("Square(7) = %d"), Square.Execute(7));
	}

	// 2. Multicast delegate. Every Add returns a handle you can remove it with.
	const FDelegateHandle NativeHandle = OnNotify.AddUObject(this, &AExDelegateZoo::NativeListener);
	OnNotify.AddLambda([](const FString& Message)
	{
		UE_LOG(LogExDelegates, Log, TEXT("Lambda listener got '%s'"), *Message);
	});
	OnNotify.Broadcast(TEXT("hello"));

	OnNotify.Remove(NativeHandle);
	UE_LOG(LogExDelegates, Log, TEXT("After Remove:"));
	OnNotify.Broadcast(TEXT("again"));

	// 3. Dynamic multicast delegate. Broadcasting with no listeners is fine.
	OnScored.AddDynamic(this, &AExDelegateZoo::HandleScored);
	OnScored.Broadcast(10);
}

void AExDelegateZoo::NativeListener(const FString& Message)
{
	UE_LOG(LogExDelegates, Log, TEXT("Native listener got '%s'"), *Message);
}

void AExDelegateZoo::HandleScored(int32 Points)
{
	UE_LOG(LogExDelegates, Log, TEXT("Dynamic listener got %d points"), Points);
}
