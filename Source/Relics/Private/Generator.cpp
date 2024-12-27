#include "Generator.h"

#include <sstream>

#include "GeneratorImpl.h"
#include "Room.h"
#include "Components/InstancedStaticMeshComponent.h"

void AGenerator::buildBasePlate()
{
	FMatrix transformMatrix = FMatrix(
		FPlane(size * 1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, size * 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 1.0f, 0.0f),
		FPlane(0.0f, 0.0f, -100.0f, 1.0f)
	);

	blocks->ClearInstances();
	blocks->AddInstance(FTransform(transformMatrix));
}

ARoom* AGenerator::build(UWorld* world, RandomGenerator& rg, const RoomImpl& room)
{
	unsigned int row = room.getRow();
	unsigned int col = room.getCol();
	FVector spawnLocation(row * 100.f, col * 100.f, 0.f);
	FTransform spawnTransform(spawnLocation);

	// Spawn the actor.
	ARoom* spawnedRoom = world->SpawnActorDeferred<ARoom>(ARoom::StaticClass(), spawnTransform, this, nullptr);
	if (spawnedRoom)
	{
		spawnedRoom->init(room, rg);

		UE_LOG(LogTemp, Log, TEXT("Spawned actor %s successfully!"), *spawnedRoom->GetName());
		return spawnedRoom;
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor."));

	return nullptr;
}

AGenerator::AGenerator()
	: size(32), room_min(5), room_max(5), gap(3), seed(0)

{
	UE_LOG(LogTemp, Log, TEXT("Constructor called"));
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

AGenerator::~AGenerator()
{
	clearRooms();
}

void AGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UWorld* world = GetWorld();

	clearRooms();

	buildBasePlate();

	GeneratorImpl generator(size, room_min, room_max, gap, seed);
	generator.generate();

	std::stringstream s;
	s << generator << std::endl;
	UE_LOG(LogTemp, Warning, TEXT("%hs"), s.str().c_str());

	for (auto room : generator.getRooms())
	{
		build(world, generator.getRandomGenerator(), room);
	}
	UE_LOG(LogTemp, Log, TEXT("OnConstruction finished"));
}

void AGenerator::clearRooms()
{
	for (auto entry : rooms)
	{
		entry->clearEnemies();
		entry->Destroy();
	}
	rooms.clear();
}
