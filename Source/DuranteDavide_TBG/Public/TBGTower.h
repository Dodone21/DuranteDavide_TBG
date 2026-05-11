// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMapManager.h"
#include "TBGGameInstance.h"
#include "TBGBaseUnit.h"
#include "GameFramework/Actor.h"
#include "TBGTower.generated.h"



UENUM(BlueprintType)
enum class ETowerState : uint8 
{
	Neutral,
	Controlled,
	Contested
};

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGTower : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ATBGTower();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

protected:

    // ----------------------------------------------------------
    // POSIZIONE GRIGLIA
    // ----------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tower")
    int32 GridX = -1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tower")
    int32 GridY = -1;

    // ----------------------------------------------------------
    // STATE
    // ----------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tower")
    ETowerState TowerState = ETowerState::Neutral;

    // Torre controllato da?
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tower")
    EUnitTurnOwner TurnControlledBy = EUnitTurnOwner::None;

    // Componente mesh per la torre (es. un cilindro o un modello 3D)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tower")
    UStaticMeshComponent* TowerMesh;

    // Materiale quando la torre è neutrale (Grigio/Bianco)
    UPROPERTY(EditAnywhere, Category = "Tower | Visual")
    UMaterialInterface* NeutralMaterial;

    // Materiale quando è controllata dall'Umano
    UPROPERTY(EditAnywhere, Category = "Tower | Visual")
    UMaterialInterface* HumanMaterial;

    // Materiale quando è controllata dall'AI
    UPROPERTY(EditAnywhere, Category = "Tower | Visual")
    UMaterialInterface* AIMaterial;

    // Materiale quando è contesa (es. un materiale a strisce o lampeggiante)
    UPROPERTY(EditAnywhere, Category = "Tower | Visual")
    UMaterialInterface* ContestedMaterial;

public:
    // ----------------------------------------------------------
    // GETTERS
    // ----------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Tower")
    int32 GetGridX() const { return GridX; }

    UFUNCTION(BlueprintCallable, Category = "Tower")
    int32 GetGridY() const { return GridY; }

    UFUNCTION(BlueprintCallable, Category = "Tower")
    ETowerState GetTowerState() const { return TowerState; }

    UFUNCTION(BlueprintCallable, Category = "Tower")
    EUnitTurnOwner GetController() const { return TurnControlledBy; }

    UFUNCTION(BlueprintCallable, Category = "Tower")
    FGridCell GetCell() const;

    UFUNCTION(BlueprintCallable, Category = "Tower")
    FIntPoint GetGridPosition() const;

    // ----------------------------------------------------------
    // SETTERS
    // ----------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Tower")
    void SetGridPosition(int32 InX, int32 InY);

    UFUNCTION(BlueprintCallable, Category = "Tower")
    void SetControlledBy(EUnitTurnOwner NewOwner);

    UFUNCTION(BlueprintCallable, Category = "Tower Logic")
    void EvaluateEEUpdateTowerControlState(const TArray<class ATBGBaseUnit*>& NearbyUnits);

    /** Ritorna la lista di unità (Player o AI) entro il raggio di 2 celle */
    TArray<ATBGBaseUnit*> GetUnitsInCaptureZone() const;

    /**
     * Spawna 3 torri sulla griglia nelle posizioni ideali definite
     * dalle specifiche (centro + laterali). Da chiamare dopo la
     * generazione della mappa e prima del posizionamento unità.
     */
    UFUNCTION(BlueprintCallable, Category = "Tower|Placement", meta = (WorldContext = "WorldContextObject"))
    static void SolvePlacement(UObject* WorldContextObject, AGridMapManager* GridManager, TSubclassOf<ATBGTower> TowerClass);

    /**
    BFS/spirale: restituisce la cella walkable più vicina al punto ideale.
    E' per spostare la location della torre nel caso in cui abbia
    EElevationLevel = 0/Water
    */
    static FIntPoint FindNearestValidTowerSpot(AGridMapManager* GridManager, FIntPoint Ideal);

    // ritorna true se l'unita' passata e' dentro il range di controllo
    bool IsUnitInsideCaptureRange(ATBGBaseUnit* Unit) const;

    // PER L'AI ED IL CONTROLLO
    UPROPERTY()
    ATBGBaseUnit* AssignedDefender = nullptr;

    bool HasAssignedDefender() const    { return AssignedDefender != nullptr; }

private:
    void UpdateVisuals();
};