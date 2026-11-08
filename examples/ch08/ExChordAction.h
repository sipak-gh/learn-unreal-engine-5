// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExChordAction.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Space alone logs "jump"; Shift + Space logs "dash". The dash mapping has a
 * Chorded Action trigger that requires the Shift action to be active.
 * Everything is created in code, like ExRuntimeInput.
 */
UCLASS()
class AExChordAction : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

private:
	void CreateInputObjects();

	void OnJump();
	void OnDash();

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ShiftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Context;
};
