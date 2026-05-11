// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TBGGameInstance.generated.h"

class ATBGBaseUnit;
class ATBGTower;
class AGridMapManager;

UENUM(BlueprintType)
enum class EElevationLevel : uint8
{
    Water = 0      UMETA(DisplayName = "0/Water"),
    Flat = 1       UMETA(DisplayName = "1/Flat"),
    MountainLow = 2  UMETA(DisplayName = "2/Mountain Low"),
    MountainMid = 3  UMETA(DisplayName = "3/Mountain Mid"),
    MountainHigh = 4  UMETA(DisplayName = "4/Mountain High")
};

// Struttura della Cella (Coordinate X, Y e Elevazione Z)
USTRUCT(BlueprintType)
struct FGridCell
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 X = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 Y = 0;

    // Rappresentazione intera dell'altezza (0-4)
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 Z = 0;

    // Z è rappresentata dal livello di elevazione quantizzato
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    EElevationLevel ElevationLevel = EElevationLevel::Flat;

    UPROPERTY(BlueprintReadWrite)
    ATBGBaseUnit* OccupyingUnit = nullptr;

    UPROPERTY(BlueprintReadWrite)
    ATBGTower* Tower = nullptr;

    UPROPERTY()
    bool bIsWater = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsTowerCell = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsOccupiedByUnit = false;

    FGridCell() {}
    FGridCell(int32 InX, int32 InY, int32 InZ) : X(InX), Y(InY), Z(InZ)
    {
        ElevationLevel = static_cast<EElevationLevel>(FMath::Clamp(InZ, 0, 4));
    }
    bool operator==(const FGridCell& Other) const { return X == Other.X && Y == Other.Y; }

};

USTRUCT(BlueprintType)
struct FMapGenerationSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") float PerlinScale = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") float HeightMultiplier = 50.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") int32 RandomSeed = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") int32 MapWidth = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") int32 MapHeight = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") int32 MaxElevation = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings") float CellSize = 100.0f;
};

// Tiene traccia a chi appartiene tutto 
UENUM(BlueprintType)
enum class EUnitTurnOwner : uint8 //EUnitTurnOwner ETurnOwner
{
    None    UMETA(DisplayName = "Nessuno"),
    Human   UMETA(DisplayName = "Giocatore Umano"),
    AI      UMETA(DisplayName = "Intelligenza Artificiale")
};


UCLASS()
class DURANTEDAVIDE_TBG_API UTBGGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    virtual void Init() override;

    AGridMapManager* GetGridMapManager() const { return GridMapManager; }

    // Inizializza il gioco 
    UFUNCTION(BlueprintCallable, Category = "Game")
    void InitializeGame(FMapGenerationSettings Settings);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    FGridCell GetGridCell(int32 X, int32 Y, int32 Z) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GetWorldPositionFromCell(const FGridCell& Cell) const;

    // Ritorna i parametri attuali della mappa 
    FMapGenerationSettings GetMapSettings() const { return CurrentMapSettings; }

    // Verifica se le coordinate X e Y rientrano nei limiti della mappa 
    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool IsValidGridPosition(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "Game")
    void RandomizeSeed();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FMapGenerationSettings CurrentMapSettings;

private:
    UPROPERTY()
    TObjectPtr<AGridMapManager> GridMapManager;
};
