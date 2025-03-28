#pragma once
#include <vector>

#include "TwoDArray.h"
#include "CoreMinimal.h"
#include "RoomImpl.h"
#include "GameFramework/Actor.h"
#include "Relics/Utils/Utils.h"
#include "Room.generated.h"

UCLASS()
class RELICS_API ARoom : public AActor
{
public:
	bool constructed;
	std::vector<AActor*> enemies;
	GENERATED_BODY()
	
	RoomImpl room;
	RandomGenerator rg;

	void build(UWorld* world);
	AActor* spawnActors(UWorld* world);

public:
	ARoom();
	~ARoom();

	void init(const RoomImpl& roomRef, RandomGenerator& rgRef);
	void buildWalls();
	void buildWall(std::vector<std::pair<int, int>>& walls);
	void buildVerticalWall(std::pair<int, int>& p1, std::pair<int, int>& p2);
	void buildHorizontalWall(std::pair<int, int>& p1, std::pair<int, int>& p2);
	void buildOverheads();
	void buildWallSegment(float r, float c, float alty, float rScale,
							 float cScale, float zScale);

	UPROPERTY(EditAnywhere)
	class UInstancedStaticMeshComponent* blocks;

	UPROPERTY(EditAnywhere)
	class UClass* enemy;
    
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 width;
    
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 height;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 alt;
	
	virtual void OnConstruction(const FTransform& Transform) override;
    
	void clearEnemies();
};