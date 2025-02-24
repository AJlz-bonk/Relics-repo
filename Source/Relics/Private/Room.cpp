#include "Room.h"
#include "../Relics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Relics/Utils/Utils.h"

#include <algorithm>

void ARoom::buildWalls()
{
	buildWall(room.getWalls());
	buildWall(room.getInteriorWalls());
}

void ARoom::buildWall(std::vector<std::pair<int, int>>& walls)
{
	bool isVert = true;
	std::pair<int, int>* p1 = nullptr;

	for (auto& p2 : walls)
	{
		if (p1)
		{
			isVert ? buildVerticalWall(*p1, p2) : buildHorizontalWall(*p1, p2);
			isVert = !isVert;
		}
		p1 = &p2;
	}
	unless(p1 == nullptr)
	{
		isVert ? buildVerticalWall(*p1, walls[0]) : buildHorizontalWall(*p1, walls[0]);
	}
}

void ARoom::buildVerticalWall(std::pair<int, int>& p1, std::pair<int, int>& p2)
{
	int row = std::min(p1.first, p2.first);
	int rScale = 0;
	int rStart = row;

	//loops over the row
	//scale & loop begins at 1 and row because corners are never doors
	for (int r = row; r < std::max(p1.first, p2.first) + 1; r++)
	{
		//if there is no door then increase the length of the wall segment
		unless(room.getDoors().contains({r, p1.second}))
		{
			rScale++;
		}
		//if there is a door then build a segment from the starting position with the pos and scale
		else
		{
			buildWallSegment(rStart, p1.second, 0, rScale, 1, /*door height*/3);
			buildWallSegment(r, p1.second, 0, 1, 1, 0.2);
			rScale = 0;
			rStart = r + 1;
		}
	}

	//builds the final segment
	//if there were no doors, builds the whole wall
	buildWallSegment(rStart, p1.second, 0, rScale, 1, /*door height*/3);
}

void ARoom::buildHorizontalWall(std::pair<int, int>& p1, std::pair<int, int>& p2)
{
	int col = std::min(p1.second, p2.second) + 1;
	int cScale = 0;
	int cStart = col;
	//loops over the col
	//scale & loop begins at 1 and row because corners are never doors
	for (int c = col; c < std::max(p1.second, p2.second); c++)
	{
		//if there is no door then increase the length of the wall segment
		unless(room.getDoors().contains({p1.first, c}))
		{
			cScale++;
		}
		//if there is a door then build a segment from the starting position with the pos and scale
		else
		{
			buildWallSegment(p1.first, cStart, 0, 1, cScale, /*door height*/3);
			buildWallSegment(p1.first, c, 0, 1, 1, 0.2);
			cScale = 0;
			cStart = c + 1;
		}
	}
	//builds the final segment
	//if there were no doors, builds the whole wall
	if (cScale > 0)
	{
		buildWallSegment(p1.first, cStart, 0, 1, cScale, /*door height*/3);
	}
}

void ARoom::buildOverheads()
{
	int r1 = -1;
	int c1 = -1;
	int r2 = height;
	int c2 = width;
	unsigned int doorHeight = 3;
	unsigned int zScale = alt - doorHeight;

	//builds four overhead walls
	buildWallSegment(r1, c1, doorHeight, height + 2, 1, zScale);
	buildWallSegment(r1, c2, doorHeight, height + 2, 1, zScale);
	//rows is increased and width is decreased so that overhead walls do not overlap
	buildWallSegment(r1, c1 + 1, doorHeight, 1, width, zScale);
	buildWallSegment(r2, c1 + 1, doorHeight, 1, width, zScale);
}

void ARoom::buildWallSegment(float r, float c, float alty, float rScale,
                             float cScale, float zScale)
{
	FMatrix transformMatrix = FMatrix(
		FPlane(rScale * 1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, cScale * 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, zScale * 1.0f, 0.0f),
		FPlane(r * 100.0f, c * 100.0f, alty * 100.0f, 1.0f)
	);

	blocks->AddInstance(FTransform(transformMatrix));
}

void ARoom::build(UWorld* world)
{
	//builds the floor
	//buildWallSegment(1, 1, 0, height - 2, width - 2, 0.2);

	//builds the ceiling
	//buildWallSegment(0, 0, alt - 0.2, height, width, 0.2);

	//builds the walls
	//buildOverheads();
	buildWalls();

	enemies.push_back(spawnEnemy(world));
	UE_LOG(LogRelics, Log, TEXT("added enemy"));
}

AActor* ARoom::spawnEnemy(UWorld* world)
{
	FVector spawnPos = GetActorLocation();
	FVector spawnLocation(spawnPos.X + height / 2.f * 100.f, spawnPos.Y + width / 2.f * 100.f, 109.f);

	// Spawn the actor.
	AActor* spawnedEnemy = world->SpawnActorDeferred<AActor>(enemy, FTransform(spawnLocation), this,
	                                                         nullptr);
	if (spawnedEnemy)
	{
		//UE_LOG(LogTemp, Log, TEXT("Spawned actor %s successfully!"), *spawnedEnemy->GetName());

		return spawnedEnemy;
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor."));

	return nullptr;
}

void ARoom::clearEnemies()
{
	//UE_LOG(LogRelics, Log, TEXT("clear %d enemies"), static_cast<int>(enemies.size()));

	for (auto entry : enemies)
	{
		entry->Destroy();
	}
	enemies.clear();

	if (blocks != nullptr)
	{
		blocks->ClearInstances();
	}
}

ARoom::ARoom()
	: constructed(false), enemies(std::vector<AActor*>()),
	  width(0), height(0)
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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find the cube mesh"));
	}

	static ConstructorHelpers::FObjectFinder<UBlueprint> foe(TEXT("/Game/EnemyStuff/BP_EnemyAI"));
	if (foe.Succeeded())
	{
		enemy = foe.Object->GeneratedClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find the enemy blueprint"));
	}
}


ARoom::~ARoom()
{
	UE_LOG(LogTemp, Warning, TEXT("ARoom destructor called"));

	/*
	delete blocks;

	for (auto entry : enemies)
	{
		entry->Destroy();
		//UE_LOG(LogRelics, Log, TEXT("Enemy destroyed!!!"));
	}
	enemies.clear();
	*/

	clearEnemies();
}

void ARoom::init(const RoomImpl& roomRef, RandomGenerator& rgRef)
{
	room = roomRef;
	rg = rgRef;
	width = roomRef.getWidth();
	height = roomRef.getHeight();
	alt = rgRef.getRandom(4, 7);
}

void ARoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (constructed)
	{
		return;
	}
	constructed = true;

	if (room.getDoors().size() == 0)
	{
		return;
	}

	//UE_LOG(LogTemp, Log, TEXT("OnConstruction called: w=%d, h=%d, doors:%d, enemyCount:%d"), width, height,
	//       static_cast<int>(doors.size()), static_cast<int>(enemies.size()));

	UWorld* world = GetWorld();

	blocks->ClearInstances();

	build(world);
}
