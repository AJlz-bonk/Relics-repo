#pragma once
#include <vector>

#include "Room.h"
#include "RoomImpl.h"
#include "NavMesh/NavMeshBoundsVolume.h"

#include "Generator.generated.h"

UCLASS(Blueprintable)
class RELICS_API AGenerator : public AActor
{
	GENERATED_BODY()
	std::vector<ARoom*> rooms;

	void clearDungeon();
	void delayedBuildNavigation();

public:
	void buildBasePlate();
	void buildNavMesh();
	void init(int32 tSize, int32 tRoom_min, int32 tRoom_max, int32 tGap, int32 tSeed);
	ARoom* build(UWorld* world, RandomGenerator& rg, const RoomImpl& room);

	AGenerator();
	~AGenerator();

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "Generator stuff")
	void buildDungeon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator stuff",
		meta = (ExposeOnSpawn = "true", ClampMin = 16))
	int32 size;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator stuff",
		meta = (ExposeOnSpawn = "true", ClampMin = 5))
	int32 room_min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator stuff",
		meta = (ExposeOnSpawn = "true", ClampMin = 5))
	int32 room_max;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator stuff",
		meta = (ExposeOnSpawn = "true", ClampMin = 3))
	int32 gap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator stuff",
		meta = (ExposeOnSpawn = "true", ClampMin = 0))
	int32 seed;

	UPROPERTY(EditAnywhere)
	UInstancedStaticMeshComponent* blocks;

	UPROPERTY(EditAnywhere)
	class UClass* enemy;

	UPROPERTY(EditAnywhere)
	class UClass* chest;

	UPROPERTY(EditAnywhere)
	class UClass* exit;

	UPROPERTY(EditAnywhere)
	class ANavMeshBoundsVolume* navMesh;
};
