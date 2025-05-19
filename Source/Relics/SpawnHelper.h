#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpawnHelper.generated.h"

UCLASS()
class RELICS_API USpawnHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintCallable, Category = "Generator stuff")
	static AActor* SpawnGeneratorInLevel(UObject* worldContextObject, TSubclassOf<AActor> actorClass, FTransform spawnTransform, int32 size, int32 room_min, int32 room_max, int32 gap, int32 seed);
	
};