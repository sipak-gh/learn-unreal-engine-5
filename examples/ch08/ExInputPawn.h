// Learn Unreal Engine 5 - chapter 8 example.
// Build.cs modules: Core, CoreUObject, Engine, InputCore, EnhancedInput.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExInputPawn.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionInstance;

/**
 * Logs every trigger event of one input action, so you can watch the
 * Started / Ongoing / Triggered / Completed / Canceled life cycle.
 * Needs two assets: a Boolean action (e.g. IA_ExJump) and a context mapping Space Bar to it.
 */
UCLASS()
class AExInputPawn : public APawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Runs whenever the controller changes: the right moment to add a mapping context. */
	virtual void NotifyControllerChanged() override;

protected:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> Context;

private:
	void OnJumpEvent(const FInputActionInstance& Instance);
};
