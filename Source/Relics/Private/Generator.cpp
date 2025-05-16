#include "Generator.h"

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
		//spawns the navmesh in the center of the dungeon
		FVector spawnLoc = FVector(size / 2.f * 100.f, size / 2.f * 100.f, 0.f);
		FActorSpawnParameters spawnParams;
		spawnParams.OverrideLevel = GetLevel();
		
		navMesh = World->SpawnActor<ANavMeshBoundsVolume>(
			ANavMeshBoundsVolume::StaticClass(),
			spawnLoc,
			FRotator::ZeroRotator,
			spawnParams
		);

		if (navMesh)
		{
			// Create the BoxComponent
			UBoxComponent* BoxComp = NewObject<UBoxComponent>(navMesh);
			if (BoxComp)
			{
				BoxComp->RegisterComponent();
				//accepts a radius, not a diameter, so divide all values by 2
				BoxComp->SetBoxExtent(FVector(size * 100.f / 2.f, size * 100.f / 2.f, 1000.f));
				BoxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				BoxComp->SetWorldLocation(navMesh->GetActorLocation());

				// Attach to actor
				BoxComp->AttachToComponent(navMesh->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

				// Tell nav system about it
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				if (NavSys)
				{
					NavSys->OnNavigationBoundsUpdated(navMesh);
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
	FActorSpawnParameters spawnParams;

	//sets the correct level
	ULevel* OwningLevel = GetLevel();
	if (!OwningLevel)
	{
		UE_LOG(LogTemp, Error, TEXT("AGenerator does not have a valid level"));
		return nullptr;
	}

	spawnParams.OverrideLevel = OwningLevel;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the actor.
	//ARoom* spawnedRoom = world->SpawnActorDeferred<ARoom>(ARoom::StaticClass(), spawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	ARoom* spawnedRoom = world->SpawnActor<ARoom>(ARoom::StaticClass(), spawnTransform, spawnParams);

	if (spawnedRoom)
	{
		spawnedRoom->init(room, rg, enemy, chest, exit);

		//UGameplayStatics::FinishSpawningActor(spawnedRoom, spawnTransform);											No longer needed because SpawnActorDeferred is no longer being used
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
		UE_LOG(LogTemp, Error, TEXT("Could not find enemy blueprint"));
	}

	static ConstructorHelpers::FObjectFinder<UBlueprint> box(TEXT("/Game/Interactables/BP_Chest"));
	if (box.Succeeded())
	{
		chest = box.Object->GeneratedClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find chest blueprint"));
	}

	static ConstructorHelpers::FObjectFinder<UBlueprint> door(TEXT("/Game/Interactables/BP_Gate"));
	if (door.Succeeded())
	{
		exit = door.Object->GeneratedClass;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find exit blueprint"));
	}
}

AGenerator::~AGenerator()
{
	clearDungeon();
}

void AGenerator::buildDungeon()
{
	UE_LOG(LogTemp, Warning, TEXT("Pre-Gen-GetWorld"));
	UWorld* world = GetWorld();
	UE_LOG(LogTemp, Warning, TEXT("Post-Gen-GetWorld"));

	clearDungeon();

	buildBasePlate();

	if (!seed)
	{
		seed = RandomGenerator().getRandom();
	}

	GeneratorImpl generator(size, room_min, room_max, gap, seed);
	generator.generate();

	for (auto room : generator.getRooms())
	{
		rooms.push_back(build(world, generator.getRandomGenerator(), room));
	}
	buildNavMesh();
}

void AGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AGenerator::BeginDestroy()
{
	UE_LOG(LogTemp, Warning, TEXT("begin destroy called"));

	clearDungeon();
	Super::BeginDestroy();
}

void AGenerator::clearDungeon()
{
	UE_LOG(LogTemp, Warning, TEXT("clearDungeon called"));

	TArray<AActor*> foundRoomActors;

	UE_LOG(LogTemp, Warning, TEXT("Pre-Gen-ClearDungeon-GetWorld"));
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARoom::StaticClass(), foundRoomActors);
	UE_LOG(LogTemp, Warning, TEXT("Post-Gen-ClearDungeon-GetWorld"));

	for (auto actor : foundRoomActors)
	{
		ARoom* room = Cast<ARoom>(actor);
		if (room)
		{
			room->clearActors();
		}
		actor->Destroy();
	}
	rooms.clear();

	TArray<AActor*> foundEnemyActors;

	UE_LOG(LogTemp, Warning, TEXT("Post-Gen-ClearDungeon-2-GetWorld"));
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), enemy, foundEnemyActors);
	UE_LOG(LogTemp, Warning, TEXT("Post-Gen-ClearDungeon-2-GetWorld"));

	for (auto enemyActor : foundEnemyActors)
	{
		if (enemyActor)
		{
			enemyActor->Destroy();
		}
	}

	if (navMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to delete navMesh actor"));

		navMesh->Destroy();
		navMesh = nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find navMesh actor"));
	}
}
