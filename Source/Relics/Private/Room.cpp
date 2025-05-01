#include "Room.h"
#include "../Relics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Relics/Utils/Utils.h"

#include <algorithm>
#include <array>
#include <unordered_set>

#include "Kismet/GameplayStatics.h"

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
			//buildWallSegment(r, p1.second, 0, 1, 1, 0.2);
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
			//buildWallSegment(p1.first, c, 0, 1, 1, 0.2);
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

struct PairHash {
	std::size_t operator()(const std::pair<int, int>& p) const {
		return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
	}
};

FVector ARoom::getRandomValidPosition()
{
	{
		// Build blocked set
		std::unordered_set<std::pair<int, int>, PairHash> blocked;

		for (const auto& wall : room.getWalls()) {
			blocked.insert(wall);
		}
		for (const auto& interior : room.getInteriorWalls()) {
			blocked.insert(interior);
		}

		// Try random attempts first (faster if map is mostly open)
		const int maxAttempts = 1000;
		for (int attempt = 0; attempt < maxAttempts; ++attempt) {
			int x = rg.getRandom(0, width);
			int y = rg.getRandom(0, height);
			std::pair<int, int> candidate = {x, y};
			if (blocked.find(candidate) == blocked.end()) {
				return FVector(x * 100.f, y * 100.f, 0.f);
			}
		}

		// Fallback: build list of valid positions
		std::vector<std::pair<int, int>> validPoints;
		for (uint32 x = 0; x <= width; ++x) {
			for (uint32 y = 0; y <= height; ++y) {
				std::pair<int, int> p = {x, y};
				if (blocked.find(p) == blocked.end()) {
					validPoints.push_back(p);
				}
			}
		}

		auto chosen = validPoints[rg.getRandom(0, static_cast<int>(validPoints.size()) - 1)];
		return FVector(chosen.first * 100.f, chosen.second * 100.f, 0.f);
	}
}

void ARoom::build(UWorld* world)
{
	//builds the ceiling
	//buildWallSegment(0, 0, alt - 0.2, height, width, 0.2);

	//builds the walls
	buildOverheads();
	buildWalls();

	FVector spawnPos = GetActorLocation();

	std::vector classes = {enemy, chest};

	if (rg.getRandom(0,10) > 8)
	{
		classes.push_back(exit);
	}
	
	for (auto classToSpawn : classes)
	{
		FVector offset = getRandomValidPosition();
		FVector result = FVector(spawnPos.X + offset.X, spawnPos.Y + offset.Y, 0.f);
	
		enemies.push_back(spawnActor(world, classToSpawn, &result));
	}
}

AActor* ARoom::spawnActor(UWorld* world, UClass* actorType, FVector* location)
{
	TArray<AActor*> tempEnemies;
	FVector spawnLocation(location->X, location->Y, 109.f);
	FTransform spawnTransform = FTransform(spawnLocation);

	// Spawn the actor.
	AActor* spawnedEnemy = world->SpawnActorDeferred<AActor>(actorType, spawnTransform, this,
	                                                         nullptr);
	if (spawnedEnemy)
	{
		UGameplayStatics::FinishSpawningActor(spawnedEnemy, spawnTransform);

		return spawnedEnemy;
	}
	
	UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor."));
	return nullptr;
}
void ARoom::clearActors()
{
	for (auto entry : enemies)
	{
		entry->Destroy();
	}
	enemies.clear();
}

ARoom::ARoom()
	: constructed(false), enemies(std::vector<AActor*>()),
	  enemy(nullptr), chest(nullptr), exit(nullptr), width(0), height(0)
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
}


ARoom::~ARoom()
{
	clearActors();
}

void ARoom::init(const RoomImpl& roomRef, RandomGenerator& rgRef, UClass* enemyRef, UClass* chestRef, UClass* exitRef)
{
	enemy = enemyRef;
	chest = chestRef;
	exit = exitRef;
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

	UWorld* world = GetWorld();

	blocks->ClearInstances();

	build(world);
}

void ARoom::BeginDestroy()
{
	clearActors();
	Super::BeginDestroy();
}
