#include "Room.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Relics/Utils/Utils.h"

void ARoom::build(UWorld* world)
{
	unsigned int alt = random_in_range(4, 7);

	//builds the floor
	buildWallSegment(0, 0, 0, height, width, 0.2);
	
	//builds the ceiling
	buildWallSegment(0, 0, alt - 0.2, height, width, 0.2);

#ifdef DEBUGGIN_DOORS
	//DISABLED calls door.build on every door to build a 1x1 cube on the inside tile of every door
	for (const auto& entry : doors)
	{
		for (const auto& door : entry.second)
		{
			door.build(blocks, *this, entry.first);
		}
	}
#endif

	//builds the walls
	buildWalls(alt);
}

void ARoom::buildWalls(unsigned int alt)
{
	/*
	   r1, c1, r2, c2 visualized:
	   
	        * r1 r1 r1 r1 *
	       c1 _  row_  _  c2
	       c1 c  _  _  _  c2
	       c1 o  _  _  _  c2
	       c1 l  _  _  _  c2
	        * r2 r2 r2 r2 *
	 */
	int row = 0;
	int col = 0;
	int r1 = -1;
	int c1 = -1;
	int r2 = height;
	int c2 = width;
	unsigned int doorHeight = 3;
	unsigned int zScale = alt - doorHeight;

	buildOverheads(r1, c1, r2, c2, doorHeight, zScale);
	
	//repeats for c1 and c2
	for (int i = c1; i <= c2; i += c2 - c1)
	{
		int rScale = 1;
		int rStart = r1;

		//loops over the row
		//scale & loop begins at 1 and row because corners are never doors
		for (int r = row; r <= r2; r++)
		{
			//if there is no door then increase the length of the wall segment
			unless(hasAtPos(r, i))
			{
				rScale++;
			}
			//if there is a door then build a segment from the starting position with the pos and scale
			else
			{
				buildWallSegment(rStart, i, 0, rScale, 1, doorHeight);
				buildWallSegment(r, i, 0, 1, 1, 0.2);
				rScale = 0;
				rStart = r + 1;
			}
		}
		//builds the final segment
		//if there were no doors, builds the whole wall
		buildWallSegment(rStart, i, 0, rScale, 1, doorHeight);
	}
	
	//repeats for r1 and r2
	for (int j = r1; j <= r2; j += r2 - r1)
	{
		int cScale = 0; 
		int cStart = col;
		//loops over the col
		//scale & loop begins at 1 and row because corners are never doors
		for (int c = col; c < c2; c++)
		{
			//if there is no door then increase the length of the wall segment
			unless(hasAtPos(j, c))
			{
				cScale++;
			}
			//if there is a door then build a segment from the starting position with the pos and scale
			else
			{
				buildWallSegment(j, cStart, 0, 1, cScale, doorHeight);
				buildWallSegment(j, c, 0, 1, 1, 0.2);
				cScale = 0;
				cStart = c + 1;
			}
		}
		//builds the final segment
		//if there were no doors, builds the whole wall
		if (cScale > 0)
		{
			buildWallSegment(j, cStart, 0, 1, cScale, doorHeight);
		}
	}
}

void ARoom::buildOverheads(float r1, float c1, float r2, float c2,
	float doorHeight, float zScale)
{
	//builds four overhead walls
	buildWallSegment(r1, c1, doorHeight, height + 2, 1, zScale);
	buildWallSegment(r1, c2, doorHeight, height + 2, 1, zScale);
	//rows is increased and width is decreased so that overhead walls do not overlap
	buildWallSegment(r1, c1 + 1, doorHeight, 1, width, zScale);
	buildWallSegment(r2, c1 + 1, doorHeight, 1, width, zScale);
}

void ARoom::buildWallSegment(float r, float c, float alt, float rScale,
	float cScale, float zScale)
{
	FMatrix transformMatrix = FMatrix(
		FPlane(rScale * 1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, cScale * 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, zScale * 1.0f, 0.0f),
		FPlane(r * 100.0f, c * 100.0f, alt * 100.0f, 1.0f)
	);

	blocks->AddInstance(FTransform(transformMatrix));
}

bool ARoom::hasAtPos(int row, int col)
{
	for (const auto& entry : doors)
	{
		for (const auto& door : entry.second)
		{
			if (door.row == row && door.col == col)
			{
				return true;
			}
		}
	}
	return false;
}

ARoom::ARoom()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);

	blocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	blocks->SetupAttachment(SceneComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube"));
	if (cubeMesh.Succeeded())
	{
		blocks->SetStaticMesh(cubeMesh.Object);
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find the cube mesh"));
	}
}

ARoom::~ARoom()
{
	blocks->ClearInstances();
	delete blocks;
}


void ARoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UWorld* world = GetWorld();

	blocks->ClearInstances();

	build(world);
}
