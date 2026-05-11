// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Kismet/GameplayStatics.h"
#include "TBGBaseUnit.h"
#include "GameFramework/GameModeBase.h"
#include "AIController.h"
#include "TBGAIController.generated.h"

class AGridMapManager;
class ATBGGameStateBase;
class ATBGameMode;
class ATBGTower;

enum class EAIObjectiveType : uint8
{
    None,
    Enemy,
    Tower,
    HoldTower
};

struct FAIObjective
{
    EAIObjectiveType Type = EAIObjectiveType::Tower;
    TObjectPtr<ATBGTower> Tower = nullptr;
    TObjectPtr<ATBGBaseUnit> Enemy = nullptr;
};

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGAIController : public AAIController
{
	GENERATED_BODY()
	
public:
    ATBGAIController();

    void ExecuteAIUnitTurn();

    bool ExecuteUnitTurn(ATBGBaseUnit* Unit);

    void StartPlacementThinking();


    void ExecutePlacement();

    void ProcessNextAIUnit();

    void CheckUnitMovementFinished(ATBGBaseUnit* Unit);

    UFUNCTION()
    void OnCurrentAIUnitActionFinished();

    UPROPERTY()
    TArray<TObjectPtr<ATBGBaseUnit>> ProcessableAIUnits;

    UPROPERTY()
    TObjectPtr<ATBGBaseUnit> CurrentAIUnit = nullptr;

    FAIObjective ChooseBestObjective(ATBGBaseUnit* SourceUnit);

protected:
    virtual void BeginPlay() override;

    void FinishUnitTurnAndProceed();
private:
    // Riferimento cached al GridMapManager
    UPROPERTY()
    AGridMapManager* GridMapManager;

        
    // ----------------------------------------------------------
    // LOGICA DI TURNO
    // ---------------------------------------------------------
    /**
     * Tenta di muovere l'unità verso l'obiettivo prioritario.
     * Priorità: torre non-AI più vicina > nemico più vicino.
     * Calcola le celle adiacenti al target, trova la migliore
     * raggiungibile.
     * Ritorna true se il movimento è avvenuto.
     */
    bool TryMove(ATBGBaseUnit* Unit);
    bool TryMoveTowardTarget(ATBGBaseUnit* Unit, const FIntPoint& TargetPos);

    /**
     * Tenta di attaccare il nemico più vicino nel range.
     * Verifica distanza Manhattan, livello di elevazione
     * Ritorna true se l'attacco è avvenuto.
     */
    bool TryAttack(ATBGBaseUnit* Unit, ATBGBaseUnit* TargetEnemy);


    // ----------------------------------------------------------
    // VALUTAZIONE OBIETTIVI
    // ----------------------------------------------------------

    /**
     * Trova la torre più vicina all'unità posseduta che
     * non sia già sotto controllo AI (Neutrale o Player).
     * Ritorna nullptr se tutte le torri sono già AI.
     */
    ATBGTower* FindClosestTargetTower(ATBGBaseUnit* SourceUnit);

    ATBGTower* FindTowerCurrentlyBeingCaptured(ATBGBaseUnit* Unit);

    /**
     * Trova l'unità nemica più vicina ancora in vita.
     * Usa distanza Manhattan per il confronto.
     * Ritorna nullptr se non esistono nemici vivi.
     */
    ATBGBaseUnit* FindClosestEnemy(ATBGBaseUnit* SourceUnit);

    FIntPoint GetFurthestReachableCellToward(ATBGBaseUnit* Unit, FIntPoint Start, FIntPoint Goal);

    //Ritorna true se appartengono a giocatori diversi.
    bool IsEnemy(ATBGBaseUnit* FirstUnit, ATBGBaseUnit* SecondUnit) const;

};
