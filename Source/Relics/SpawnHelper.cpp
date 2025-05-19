#include "SpawnHelper.h"

#include "Generator.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

AActor* USpawnHelper::SpawnGeneratorInLevel(UObject* worldContextObject, TSubclassOf<AActor> actorClass,
                                            FTransform spawnTransform, int32 size, int32 room_min, int32 room_max,
                                            int32 gap, int32 seed)
{
	if (!worldContextObject || !actorClass)
	{
		return nullptr;
	}

	UWorld* world = GEngine->GetWorldFromContextObjectChecked(worldContextObject);
	if (!world)
	{
		return nullptr;
	}

	ULevel* owningLevel = nullptr;

	if (AActor* ContextActor = Cast<AActor>(worldContextObject))
	{
		owningLevel = ContextActor->GetLevel();
	}

	if (!owningLevel)
	{
		owningLevel = world->PersistentLevel;
	}

	FActorSpawnParameters spawnParams;
	spawnParams.OverrideLevel = owningLevel;
	spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* actor = world->SpawnActor<AActor>(actorClass, spawnTransform, spawnParams);
	if (actor)
	{
		AGenerator* generator = Cast<AGenerator>(actor);
		if (generator)
		{
			generator->init(size, room_min, room_max, gap, seed);
		}
	}
	return actor;
}
