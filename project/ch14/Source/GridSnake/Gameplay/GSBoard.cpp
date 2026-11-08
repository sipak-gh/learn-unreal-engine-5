// GridSnake - course project for "Learn Unreal Engine 5".

#include "Gameplay/GSBoard.h"
#include "GridSnake.h"
#include "Camera/CameraComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "PaperTileLayer.h"
#include "PaperTileMap.h"
#include "PaperTileMapComponent.h"

AGSBoard::AGSBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMapComponent = CreateDefaultSubobject<UPaperTileMapComponent>(TEXT("TileMap"));
	SetRootComponent(TileMapComponent);
	// Tiles with collision in the tile set become blocking geometry. Block the
	// Grid channel explicitly so wall cells are found by IsCellBlocked().
	TileMapComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TileMapComponent->SetCollisionResponseToChannel(ECC_GSGrid, ECR_Block);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(TileMapComponent);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// Paper2D draws sprites in the X-Z plane. Yaw -90 makes the camera look down
	// -Y, so +X is screen right and +Z is screen up.
	Camera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void AGSBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Re-frame whenever the actor or its tile map changes in the editor.
	FrameCamera();
}

void AGSBoard::BeginPlay()
{
	Super::BeginPlay();

	const FIntPoint Size = GetGridSize();
	UE_LOG(LogGridSnake, Log, TEXT("Board %s: %dx%d cells of %.0f units"), *GetName(), Size.X, Size.Y, GetCellSize());
}

FIntPoint AGSBoard::GetGridSize() const
{
	int32 Width = 0;
	int32 Height = 0;
	int32 NumLayers = 0;
	TileMapComponent->GetMapSize(Width, Height, NumLayers);
	return FIntPoint(Width, Height);
}

float AGSBoard::GetCellSize() const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	return TileMap ? TileMap->TileWidth * TileMap->GetUnrealUnitsPerPixel() : 0.0f;
}

bool AGSBoard::IsInside(FIntPoint Cell) const
{
	const FIntPoint Size = GetGridSize();
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size.X && Cell.Y < Size.Y;
}

FVector AGSBoard::CellToWorld(FIntPoint Cell) const
{
	return TileMapComponent->GetTileCenterPosition(Cell.X, Cell.Y, /*LayerIndex=*/0, /*bWorldSpace=*/true);
}

FIntPoint AGSBoard::WorldToCell(const FVector& WorldLocation) const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	const FVector LocalLocation = TileMapComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);
	FIntPoint Cell;
	TileMap->GetTileCoordinatesFromLocalSpacePosition(LocalLocation, Cell.X, Cell.Y);
	return Cell;
}

bool AGSBoard::IsWallTile(FIntPoint Cell) const
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap || !IsInside(Cell))
	{
		return false;
	}

	for (const UPaperTileLayer* Layer : TileMap->TileLayers)
	{
		if (Layer && Layer->GetLayerCollides() && Layer->GetCell(Cell.X, Cell.Y).IsValid())
		{
			return true;
		}
	}
	return false;
}

bool AGSBoard::IsCellBlocked(FIntPoint Cell) const
{
	// A small box in the middle of the cell, deep enough (Y) to reach every layer.
	const float HalfSize = GetCellSize() * 0.25f;
	const FCollisionShape Probe = FCollisionShape::MakeBox(FVector(HalfSize, 50.0f, HalfSize));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GSCellBlocked), /*bTraceComplex=*/false);
	return GetWorld()->OverlapBlockingTestByChannel(CellToWorld(Cell), FQuat::Identity, ECC_GSGrid, Probe, Params);
}

FIntPoint AGSBoard::SnapActorToCell(AActor* Actor) const
{
	if (!Actor)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	const FIntPoint Cell = WorldToCell(Actor->GetActorLocation());
	Actor->SetActorLocation(CellToWorld(Cell));
	return Cell;
}

void AGSBoard::FrameCamera()
{
	const UPaperTileMap* TileMap = TileMapComponent->TileMap;
	if (!TileMap || TileMap->MapWidth <= 0 || TileMap->MapHeight <= 0)
	{
		return;
	}

	// Centre the camera between the first and last tile, in the actor's local space.
	const FVector FirstTile = TileMapComponent->GetTileCenterPosition(0, 0, 0, /*bWorldSpace=*/false);
	const FVector LastTile = TileMapComponent->GetTileCenterPosition(TileMap->MapWidth - 1, TileMap->MapHeight - 1, 0, /*bWorldSpace=*/false);
	const FVector Centre = (FirstTile + LastTile) * 0.5f;
	Camera->SetRelativeLocation(FVector(Centre.X, CameraDistance, Centre.Z));

	// Ortho Width is the visible width in world units. Fit whichever of the
	// map's width or height is the tighter fit for the target aspect ratio.
	const float UnitsPerPixel = TileMap->GetUnrealUnitsPerPixel();
	const float MapWidth = TileMap->MapWidth * TileMap->TileWidth * UnitsPerPixel;
	const float MapHeight = TileMap->MapHeight * TileMap->TileHeight * UnitsPerPixel;
	Camera->SetOrthoWidth(FMath::Max(MapWidth, MapHeight * TargetAspectRatio) * FramingMargin);
}
