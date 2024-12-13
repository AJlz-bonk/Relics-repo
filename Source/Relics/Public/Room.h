// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Door.h"
#include "Room.generated.h"

UCLASS()
class RELICS_API ARoom : public AActor
{
	bool constructed;
	std::vector<AActor*> enemies;
	GENERATED_BODY()

public:
	ARoom();

	~ARoom();

	UPROPERTY(EditAnywhere)
	class UInstancedStaticMeshComponent* blocks;

	UPROPERTY(EditAnywhere)
	class UClass* enemy;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 height;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 width;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	uint32 alt;

	std::map<std::string, std::vector<Door>> doors;

	virtual void OnConstruction(const FTransform& Transform) override;

	void clearEnemies();

private:
	void build(UWorld* world);

	void buildWalls(UWorld* world);

	void buildOverheads(float r1, float c1, float r2, float c2, float doorHeight, float zScale);

	void buildWallSegment(float r, float c, float alt, float rScale, float cScale, float zScale);
	
	bool hasAtPos(int row, int col);

	AActor* spawnEnemy(UWorld* world);
};