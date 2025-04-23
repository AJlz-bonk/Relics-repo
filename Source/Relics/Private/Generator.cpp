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
	UWorld* World = GetWorld();
	if (World)
	{
		FVector spawnLoc = FVector(size / 2.f * 100.f, size / 2.f * 100.f, 0.f);
		FActorSpawnParameters SpawnParams;
		ANavMeshBoundsVolume* NavVolume = World->SpawnActor<ANavMeshBoundsVolume>(
			ANavMeshBoundsVolume::StaticClass(),
			spawnLoc,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (NavVolume)
		{
			// Create the BoxComponent
			UBoxComponent* BoxComp = NewObject<UBoxComponent>(NavVolume);
			if (BoxComp)
			{
				BoxComp->RegisterComponent(); // VERY IMPORTANT
				BoxComp->SetBoxExtent(FVector(size * 100.f / 2.f, size * 100.f / 2.f, 1000.f)); // half-extents = 1000x1000x20 total
				BoxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				BoxComp->SetWorldLocation(NavVolume->GetActorLocation());

				// Attach to actor
				BoxComp->AttachToComponent(NavVolume->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

				// Tell nav system about it
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				if (NavSys)
				{
					NavSys->OnNavigationBoundsUpdated(NavVolume);
				}
			}
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

	buildNavMesh();

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
