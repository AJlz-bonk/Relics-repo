#pragma once
#include <vector>

#include "Room.h"
#include "RoomImpl.h"
#include "NavMesh/NavMeshBoundsVolume.h"

#include "Generator.generated.h"

UCLASS()
class RELICS_API AGenerator : public AActor
{
	GENERATED_BODY()
	std::vector<ARoom*> rooms;

	void clearRooms();

public:
	void buildBasePlate();
	void buildNavMesh();
	ARoom* build(UWorld* world, RandomGenerator& rg, const RoomImpl& room);

	AGenerator();
	~AGenerator();
	void OnConstruction(const FTransform& Transform);

	UPROPERTY(EditAnywhere, meta = (ClampMin = 16))
	uint32 size;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 5))
	uint32 room_min;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 5))
	uint32 room_max;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 3))
	uint32 gap;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	uint32 seed;
	
	UPROPERTY(EditAnywhere)
	UInstancedStaticMeshComponent* blocks;

	UPROPERTY(EditAnywhere)
	class UClass* enemy;

	UPROPERTY(EditAnywhere)
	class ANavMeshBoundsVolume* navMesh;
};
