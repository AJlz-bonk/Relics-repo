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
	static AActor* SpawnActorInSameLevel(UObject* worldContextObject, TSubclassOf<AActor> actorClass, FTransform spawnTransform);
	
};
