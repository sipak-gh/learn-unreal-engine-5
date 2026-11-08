// Learn Unreal Engine 5 - chapter 7 example.

#include "ExPossessionPawn.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogExPossess, Log, All);

void AExPossessionPawn::PossessedBy(AController* NewController)
{
	// Super sets Controller, so GetController() is valid after this line.
	Super::PossessedBy(NewController);
	UE_LOG(LogExPossess, Log, TEXT("%s possessed by %s"), *GetName(), *GetNameSafe(GetController()));
}

void AExPossessionPawn::UnPossessed()
{
	// Log before Super: Super clears Controller.
	UE_LOG(LogExPossess, Log, TEXT("%s unpossessed (controller was %s)"), *GetName(), *GetNameSafe(GetController()));
	Super::UnPossessed();
}

void AExPossessionPawn::Release()
{
	if (AController* MyController = GetController())
	{
		UE_LOG(LogExPossess, Log, TEXT("%s asks %s to let go"), *GetName(), *MyController->GetName());
		MyController->UnPossess();
	}
	else
	{
		UE_LOG(LogExPossess, Log, TEXT("%s has no controller"), *GetName());
	}
}
