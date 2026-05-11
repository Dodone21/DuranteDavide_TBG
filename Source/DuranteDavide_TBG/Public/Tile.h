// Fill out your copyright notice in the Description page of Project Settings.
//DODO
#pragma once

#include "CoreMinimal.h"
#include "TBGGameInstance.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

class AGridMapManager;

UCLASS()
class DURANTEDAVIDE_TBG_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATile();

protected:
    virtual void BeginPlay() override;

public:
    // --- DATI DELLA GRIGLIA ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | Grid")
    int32 GridX = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | Grid")
    int32 GridY = -1;

    // Il livello di altezza da 0 a 4
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | Grid")
    EElevationLevel Elevation;

protected:
    // --- COMPONENTI VISIVI ---
    // La forma della Tile
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | Visual")
    UStaticMeshComponent* TileMesh;

    // Mappa per associare il livello di elevazione al colore specifico (0=blu, 1=verde...)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile | Visual")
    TMap<EElevationLevel, UMaterialInterface*> ElevationMaterials;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | State")
    bool bIsHighlighted = false;

    // --- STATO DELLA CELLA ---
    // Indica se c'è un'unità, una torre, o se è un ostacolo di base (come l'acqua al livello 0)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile | State")
    bool bIsOccupied = false;

public:
    // Metodo per inizializzare la Tile alla generazione della mappa
    // gridmapmanager verrà castato da gameinstance che chiamerà ogni singola tile in posizione
    // GInstance --> GridMapManager(farà il perlingNoise e genera mappa --> Tile(625 Tiles) speriamo non scoppi
    UFUNCTION(BlueprintCallable, Category = "Tile | Setup")
    void SetGridPosition(int32 InX, int32 InY);
    //void SetGridPosition(int32 InX, int32 InY, EElevationLevel InElevation);

    // ----------------------------------------------------------
    // GETTERS/SETTERS
    // ----------------------------------------------------------

    UFUNCTION(BlueprintCallable)
    FGridCell GetCell() const;

    UFUNCTION(BlueprintPure, Category = "Tile | Grid")
    int32 GetGridX() const { return GridX; }

    UFUNCTION(BlueprintPure, Category = "Tile | Grid")
    int32 GetGridY() const { return GridY; }

    UFUNCTION(BlueprintCallable, Category = "Tile | Visual")
    void SetHighlight(bool bEnable);


    FIntPoint GetGridPosition();
    // ----------------------------------------------------------
    // QUERY SULLE CELLE
    // ----------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Tile | State")
    bool IsOccupied() const { return bIsOccupied; }

    UFUNCTION(BlueprintPure, Category = "Tile | State")
    bool IsHighlighted() const { return bIsHighlighted; }


    // Override dell'interfaccia
    //virtual void OnInteract_Implementation() override;
    //virtual void ToggleHighlight_Implementation(bool bIsVisible) override;
    // FATTO DIREI //virtual FIntPoint GetGridPosition_Implementation() const override;

    // Funzione interna per aggiornare il materiale visivo in base all'elevazione (colori)
    // che cambiano da (Elevation --> Highlighted --> TowerState)
    // UpdateVisual e' implemenetata su BluePrint
    // (da implementare il caso HIGHLIGHT)
    UFUNCTION(BlueprintImplementableEvent, Category = "Visuals")
    void UpdateVisuals();

};
