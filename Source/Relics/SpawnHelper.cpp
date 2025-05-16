#include "SpawnHelper.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

AActor* USpawnHelper::SpawnActorInSameLevel(UObject* worldContextObject, TSubclassOf<AActor> actorClass,
                                            FTransform spawnTransform)
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

	return world->SpawnActor<AActor>(actorClass, spawnTransform, spawnParams);
}
