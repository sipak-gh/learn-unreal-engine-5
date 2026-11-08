// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Snake/GSGridTypes.h"
#include "GSSnake.generated.h"

class AGSBoard;
class UBoxComponent;
class UInputAction;
class UPaperFlipbookComponent;
class UPaperGroupedSpriteComponent;
class UPaperSprite;
struct FInputActionValue;

/**
 * The player's pawn. The actor sits on the head's cell; the body is a list of
 * cells drawn by one grouped sprite component. The snake moves one cell every
 * StepInterval seconds once StartMoving() is called.
 */
UCLASS()
class AGSSnake : public APawn
{
	GENERATED_BODY()

public:
	AGSSnake();

	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Starts stepping (the game mode calls this when the countdown ends). */
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StartMoving();

	/** Adds segments; they appear one per step at the tail. */
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void Grow(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "Snake")
	EGSDirection GetDirection() const { return CurrentDirection; }

	UFUNCTION(BlueprintPure, Category = "Snake")
	FIntPoint GetHeadCell() const { return Segments.Num() > 0 ? Segments[0] : FIntPoint::ZeroValue; }

	UFUNCTION(BlueprintPure, Category = "Snake")
	int32 GetLength() const { return Segments.Num(); }

	UFUNCTION(BlueprintPure, Category = "Snake")
	bool IsAlive() const { return bAlive; }

	/** Console: GSGrow 5 */
	UFUNCTION(Exec)
	void GSGrow(int32 Amount);

protected:
	virtual void BeginPlay() override;

	/** Root. A query-only box that overlaps gems and exits in the head's cell. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperFlipbookComponent> Head;

	/** Every body segment is one instance of this component: one draw call for the whole tail. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPaperGroupedSpriteComponent> Body;

	/** IA_Steer: an Axis2D action. Keys and sticks are mapped to it in IMC_Snake. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SteerAction;

	/** Sprite drawn for each body segment. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TObjectPtr<UPaperSprite> BodySprite;

	/** Segments at the start of a level, head included. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake", meta = (ClampMin = "1"))
	int32 InitialLength = 3;

	/** Turns typed faster than the snake steps are remembered, up to this many. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake", meta = (ClampMin = "1"))
	int32 MaxQueuedTurns = 2;

	/** Seconds per step. Set from UGSSettings when the level starts. */
	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	float StepInterval = 0.15f;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<AGSBoard> Board;

	/** Cells occupied by the snake. [0] is the head, the last entry is the tail tip. */
	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TArray<FIntPoint> Segments;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	EGSDirection CurrentDirection = EGSDirection::Right;

private:
	void OnSteer(const FInputActionValue& Value);

	/** Adds a turn to the queue unless it is pointless or would reverse the snake. */
	void QueueDirection(EGSDirection NewDirection);

	/** Moves the snake by one cell: the heart of the game. */
	void Step();

	void Die(const TCHAR* Reason);

	/** Lays out the starting segments behind the cell the snake was spawned in. */
	void PlaceOnBoard();

	/** Moves the actor and every body sprite to match Segments. */
	void UpdateVisuals();

	TArray<EGSDirection> DirectionQueue;
	float StepAccumulator = 0.0f;
	int32 PendingGrowth = 0;
	bool bMoving = false;
	bool bAlive = true;
};
