// Fill out your copyright notice in the Description page of Project Settings.

#include "TBGGameInstance.h"
#include "GridMapManager.h"
#include "TBGTower.h"
#include "TBGBaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UTBGGameInstance::Init()
{
	UGameInstance::Init();
    RandomizeSeed();
}

void UTBGGameInstance::InitializeGame(FMapGenerationSettings Settings)
{
    CurrentMapSettings = Settings;
    if (CurrentMapSettings.RandomSeed == 0) RandomizeSeed();    
    
    GridMapManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));

    if (GridMapManager)
    {
        // Chiamiamo la generazione effettiva
        GridMapManager->GenerateGrid();
        UE_LOG(LogTemp, Log, TEXT("GameInstance: Griglia Inizializzata."));
    }
}

FGridCell UTBGGameInstance::GetGridCell(int32 X, int32 Y, int32 Z) const
{
    if (GridMapManager && IsValidGridPosition(X, Y))
    {
        return GridMapManager->GetCell(X, Y);
    }
    return FGridCell(X, Y, Z);
}

FVector UTBGGameInstance::GetWorldPositionFromCell(const FGridCell& Cell) const 
{
    if (GridMapManager)
    {
        return GridMapManager->GridToWorld(Cell.X, Cell.Y);
    }
    return FVector(Cell.X * CurrentMapSettings.CellSize, Cell.Y * CurrentMapSettings.CellSize, 0.0f);
}

bool UTBGGameInstance::IsValidGridPosition(int32 X, int32 Y) const
{
	if (X < 0 || Y < 0) return false;
	return (X < CurrentMapSettings.MapWidth && Y < CurrentMapSettings.MapHeight);
}

void UTBGGameInstance::RandomizeSeed()
{
    // Lo castiamo a int32 per il tuo RandomSeed.
    int32 NewSeed = static_cast<int32>(FDateTime::Now().GetTicks());
    CurrentMapSettings.RandomSeed = NewSeed;

    UE_LOG(LogTemp, Log, TEXT("TBGGameInstance: Nuovo Seed generato -> %d"), NewSeed);
}
