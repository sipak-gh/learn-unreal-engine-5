// GridSnake - course project for "Learn Unreal Engine 5".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Snake/GSGridTypes.h"
#include "GSSnake.generated.h"

class AGSBoard;
class UBoxComponent;
class UCurveFloat;
class UInputAction;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperGroupedSpriteComponent;
class UPaperSprite;
class USoundBase;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSSnakeDiedSignature, AGSSnake*, Snake);

/** The head's animation states. Each one maps to a flipbook asset. */
UENUM(BlueprintType)
enum class EGSHeadAnim : uint8
{
	Idle,
	Move,
	Eat,
	Dead
};

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

	/** Freezes the snake in place (used when the level is complete). */
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StopMoving();

	/** Sets the speed for this level. Curve (optional): X = length, Y = interval multiplier. */
	void ConfigureSpeed(float BaseInterval, UCurveFloat* Curve);

	/** Seconds per step right now, after applying the speed curve. */
	UFUNCTION(BlueprintPure, Category = "Snake")
	float GetStepInterval() const;

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

	UFUNCTION(BlueprintPure, Category = "Snake")
	EGSHeadAnim GetHeadAnim() const { return HeadAnim; }

	/** Fires once, when the snake hits a wall or itself. */
	UPROPERTY(BlueprintAssignable, Category = "Snake")
	FGSSnakeDiedSignature OnDied;

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

	/** Looping: before the countdown ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	/** Looping: while moving. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> MoveFlipbook;

	/** Played once when the snake grows, then back to Move. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> EatFlipbook;

	/** Played once on death; the last frame stays. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UPaperFlipbook> DeadFlipbook;

	/** Tint of the segment behind the head. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FLinearColor BodyHeadColor = FLinearColor::White;

	/** Tint of the tail tip; segments in between blend. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	FLinearColor BodyTailColor = FLinearColor(0.35f, 0.55f, 0.35f);

	/** Played when the snake dies (SW_Die). */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> DeathSound;

	/** How often the body blinks after death. */
	UPROPERTY(EditDefaultsOnly, Category = "Animation", meta = (ClampMin = "0"))
	int32 DeathBlinks = 4;

	/** Segments at the start of a level, head included. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake", meta = (ClampMin = "1"))
	int32 InitialLength = 3;

	/** Turns typed faster than the snake steps are remembered, up to this many. */
	UPROPERTY(EditDefaultsOnly, Category = "Snake", meta = (ClampMin = "1"))
	int32 MaxQueuedTurns = 2;

	/** Seconds per step at the starting length. Set by the game mode from the level list. */
	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	float BaseStepInterval = 0.15f;

	UPROPERTY(VisibleInstanceOnly, Category = "Snake")
	TObjectPtr<UCurveFloat> SpeedCurve;

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

	FLinearColor GetSegmentColor(int32 SegmentIndex) const;

	void PlayHeadAnim(EGSHeadAnim NewAnim);

	/** Bound to the head flipbook's OnFinishedPlaying: one-shot animations end here. */
	UFUNCTION()
	void HandleHeadAnimFinished();

	void ToggleBodyVisibility();

	TArray<EGSDirection> DirectionQueue;
	FTimerHandle DeathBlinkTimer;
	int32 DeathBlinksLeft = 0;
	EGSHeadAnim HeadAnim = EGSHeadAnim::Idle;
	float StepAccumulator = 0.0f;
	int32 PendingGrowth = 0;
	bool bMoving = false;
	bool bAlive = true;
};
