// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGGameInstance.h"
#include "GameFramework/Actor.h"
#include "GridMapManager.generated.h"

// Forward declarations
class ATile;
class ATBGTower;
class ATBGBaseUnit;
class ATBGCamera;
class UTBGGameInstance;

UCLASS()
class DURANTEDAVIDE_TBG_API AGridMapManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridMapManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// ----------------------------------------------------------
	// 1. GENERAZIONE GRIGLIA
	// ----------------------------------------------------------
	/*
	 * Entry point principale. Legge FMapGenerationSettings da
	 * GameInstance, genera le FGridCell con Perlin Noise,
	 * spawna gli ATBGTile visivi, valida la connettività,
	 * poi chiama SpawnTowers().
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateGrid();

	// ----------------------------------------------------------
	// QUERY/GETTERS SULLE CELLE E MONDO
	// ----------------------------------------------------------
	// Ritorna true se (X,Y) è dentro i bounds della griglia 
	bool IsValidCell(int32 InX, int32 InY) const;
	// Ritorna true se la cella è calpestabile: ElevationLevel > 0, nessuna torre, nessuna unità.
	bool IsWalkable(int32 X, int32 Y) const;
	// Ritorna true se una torre occupa la cella 
 	bool IsTowerAt(int32 InX, int32 InY) const;
	int32 GetHeight(int32 InX, int32 InY) const;
	// Ritorna il livello di elevazione (0-4) 
	EElevationLevel GetElevationLevel(int32 InX, int32 InY) const;
	int32 GetIDCell(int32 InX, int32 InY) const;
	// Ritorna la copia della FGridCell per la cella (X,Y) 
	FGridCell GetCell(int32 X, int32 Y) const;
	ATile* GetTile(int32 X, int32 Y) const;

	int32 GetMapWidth()  const { return GridSettings.MapWidth; }
	int32 GetMapHeight() const { return GridSettings.MapHeight; }
	float GetCellSize()  const { return GridSettings.CellSize; }


	// --- UI/Visual --- NON USATO
	void HighlightRange(ATBGBaseUnit* Unit);
	void ClearHighlights();

	// ----------------------------------------------------------
	// 3. UTILITY
	// ----------------------------------------------------------
	/** Converte coordinate griglia in posizione mondo 3D */
	FVector GridToWorld(int32 X, int32 Y) const;

	// NON USATO
	FString GetAlphaNumericCoords(int32 X, int32 Y) const;

	// ----------------------------------------------------------
	// UTILITY SUL MOVIMENTO
	// ----------------------------------------------------------
	/**
	* Calcola il percorso migliore usando algoritmo A*.
	* @param Start Cella di partenza
	* @param End Cella di arrivo
	* @param OutTotalCost (OUT) Verrà popolato con il costo totale. Sarà -1 se nessun percorso è trovato.
	* @return Array di coordinate che formano il percorso (vuoto se non trovato).
	*/
	TArray<FIntPoint> GetPath(FIntPoint Start, FIntPoint End, int32& OutTotalCost) const;

	// Calcola il costo di movimento da una cella all'adiacente. Piano / discesa = 1, salita = 2.
	UFUNCTION()
	int32 CalculateStepCost(FIntPoint From, FIntPoint To) const;

	// ----------------------------------------------------------
	// 4. GESTIONE UNITÀ E TORRI (setter/getter)
	// ----------------------------------------------------------
	void SetUnit(int32 X, int32 Y, ATBGBaseUnit* Unit);
	void ClearUnit(int32 X, int32 Y);
	ATBGBaseUnit* GetUnit(int32 X, int32 Y) const;
	void SetTower(int32 X, int32 Y, ATBGTower* Tower);

	// Per le UNITA' DI SPAWNING Verifica se una coordinata Y rientra nella zona di schieramento consentita.
	UFUNCTION(BlueprintPure, Category = "TBG|Grid")
	bool IsInDeploymentZone(int32 Y, bool bIsHumanUnit) const;

protected:
	// Classi Blueprint da assegnare nell'editor
	UPROPERTY(EditAnywhere, Category = "Grid|Classes")
	TSubclassOf<ATile> TileClass;

	UPROPERTY(EditAnywhere, Category = "Grid|Classes")
	TSubclassOf<ATBGTower> TowerClass;

private:
	// ----------------------------------------------------------
	// PARAMETRI DI GENERAZIONE
	// Letti da FMapGenerationSettings in GameInstance.
	// Non hardcoded qui: vengono copiati in cache all'inizio
	// di GenerateGrid() per non chiamare GameInstance ogni frame.
	// ----------------------------------------------------------
	UPROPERTY()
	FMapGenerationSettings GridSettings;

	// Array piatto [Y * MapWidth + X] delle celle logiche
	UPROPERTY()
	TArray<FGridCell> Grid;

	// Array degli attori visivi ATile (uno per cella)
	UPROPERTY()
	TArray<TObjectPtr<ATile>> Tiles;

	// Metodo che parte da un punto della mappa e controlla se tutti i punti della mappa sono raggiungibili
	UFUNCTION()
	void EnsureReachabilityOfMap(int32 StartX, int32 StartY);
};