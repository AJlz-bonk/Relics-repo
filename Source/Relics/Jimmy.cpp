// Fill out your copyright notice in the Description page of Project Settings.


#include "Jimmy.h"

// Sets default values
AJimmy::AJimmy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AJimmy::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AJimmy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AJimmy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

