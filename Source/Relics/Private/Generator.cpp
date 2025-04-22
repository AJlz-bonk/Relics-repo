#include "Generator.h"

#include <sstream>

#include "GeneratorImpl.h"
#include "NavigationSystem.h"
#include "Room.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavMesh/NavMeshBoundsVolume.h"


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

void AGenerator::buildNavMesh()
{
	float SafeSize = FMath::Max(static_cast<uint32>(1), size); // ensure minimum value
	FVector SpawnLocation(0.f, 0.f, 0.f);

	// BoxExtent is half-size in UE terms
	FVector BoxExtent(SafeSize * 100.f, SafeSize * 100.f, 100.f); // 200cm tall, adjust if needed

	UWorld* World = GetWorld();
	if (World)
	{
		FActorSpawnParameters SpawnParams;
		ANavMeshBoundsVolume* NavVolume = World->SpawnActor<ANavMeshBoundsVolume>(
			ANavMeshBoundsVolume::StaticClass(),
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (NavVolume)
		{
			// Set actual bounds through BoxComponent
			UBoxComponent* BoxComp = Cast<UBoxComponent>(NavVolume->GetRootComponent());
			if (BoxComp)
			{
				BoxComp->SetBoxExtent(BoxExtent); // Size in cm, not scale
				BoxComp->SetWorldLocation(SpawnLocation); // update position
				BoxComp->UpdateBounds();
			}

			// No scale setting needed!
			// Register with nav system
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (NavSys)
			{
				NavSys->OnNavigationBoundsUpdated(NavVolume);
			}

			navMesh = NavVolume;
		}
	}
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
		spawnedRoom->init(room, rg, enemy);
		spawnedRoom->OnConstruction(spawnTransform);
		
		UGameplayStatics::FinishSpawningActor(spawnedRoom, spawnTransform);
		return spawnedRoom;
	}
	UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor."));

	return nullptr;
}

AGenerator::AGenerator()
	: size(32), room_min(5), room_max(5), gap(3), seed(0), navMesh(nullptr)

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

	if (!seed)
	{
		seed = RandomGenerator().getRandom();
	}

	GeneratorImpl generator(size, room_min, room_max, gap, seed);
	generator.generate();

	/*
	std::stringstream s;
	s << generator << std::endl;
	UE_LOG(LogTemp, Warning, TEXT("%hs"), s.str().c_str());
	*/
	
	for (auto room : generator.getRooms())
	{
		rooms.push_back(build(world, generator.getRandomGenerator(), room));
	}

	buildNavMesh();
}

void AGenerator::clearRooms()
{
	TArray<AActor*> foundRoomActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoom::StaticClass(), foundRoomActors);

	for (auto actor : foundRoomActors)
	{
		ARoom* room = Cast<ARoom>(actor);
		if (room)
		{
			room->clearEnemies();
		}
		actor->Destroy();
	}
	rooms.clear();

	TArray<AActor*> foundEnemyActors;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), enemy, foundEnemyActors);

	for (auto enemyActor : foundEnemyActors)
	{
		if (enemyActor)
		{
			enemyActor->Destroy();
		}
	}

	if (navMesh)
	{
		navMesh->Destroy();
		navMesh = nullptr;
	}
}
