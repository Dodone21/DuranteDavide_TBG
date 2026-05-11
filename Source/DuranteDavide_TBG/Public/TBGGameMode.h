// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGGameStateBase.h"
#include "TBGBaseUnit.h"
#include "GameFramework/GameModeBase.h"
#include "TBGGameMode.generated.h"

// Forward declarations
class AGridMapManager;
class ATBGTower;
class ATBGUnitSniper;
class ATBGUnitBrawler;
class ATBGPlayerController;
class ATBGAIController;
class ATBGHUD;


//  EMatchPhase
//  Macchina a stati interna di GameMode.
UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
    WaitingForConfig    UMETA(DisplayName = "Waiting For Config"),
    CoinFlip            UMETA(DisplayName = "Coin Flip"),
    UnitPlacement       UMETA(DisplayName = "Unit Placement"),
    PlayerTurn          UMETA(DisplayName = "Player Turn"),
    AITurn              UMETA(DisplayName = "AI Turn"),
    EndMatch            UMETA(DisplayName = "End Match")
};

//  EPlacementSlot
//  Tiene traccia di quale unità deve essere piazzata.
UENUM(BlueprintType)
enum class EPlacementSlot : uint8
{
    FirstPlayer_First   UMETA(DisplayName = "First Player - First Unit"),
    SecondPlayer_First  UMETA(DisplayName = "Second Player - First Unit"),
    FirstPlayer_Second  UMETA(DisplayName = "First Player - Second Unit"),
    SecondPlayer_Second UMETA(DisplayName = "Second Player - Second Unit"),
    Complete            UMETA(DisplayName = "Placement Complete")
};

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
    ATBGGameMode();

protected:
    virtual void BeginPlay() override;

public:
    // FLUSSO PRINCIPALE & INIZIALIZZAZIONE
    /** Entry point: legge configurazione, spawna GridManager, avvia StartCoinFlip() */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void InitMatch();

    /** Esegue il coin flip casuale e setta bHumanGoesFirst */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void StartCoinFlip();

    /** Chiude la partita e notifica il vincitore */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void EndMatch(EUnitTurnOwner Winner);

    UPROPERTY()
    EUnitTurnOwner MatchWinner;
    // -------------------------------
    // FASE DI PIAZZAMENTO (PLACEMENT)
    /** Inizializza PlacementSlot e notifica il primo controller */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void StartPlacementPhase();

    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void TryUnitPlacement(int32 X, int32 Y, TSubclassOf<ATBGBaseUnit> UnitClassToSpawn);

    /** Avanza al prossimo slot di piazzamento */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void AdvancePlacementSlot();

    /** Notifica il controller corrente che deve piazzare */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void NotifyCurrentPlacementController();

    // -------------------------------
    // LOGICA DEI TURNI E AZIONI
    /** Avvia il turno (Player o AI) e resetta le azioni unità */
    //UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    //void StartTurn();

    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void StartTurnGame(EUnitTurnOwner TurnOwner, bool bIsStartOfMatch = false);

    /** Chiamata quando un'unità finisce di muoversi/attaccare */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void OnUnitActionComplete(ATBGBaseUnit* Unit);

    /** Chiamata quando tutte le unità hanno agito */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void OnTurnComplete();

    /** Controllo automatico fine turno */
    UFUNCTION(BlueprintCallable, Category = "TBG |Match")
    void CheckPlayerTurnEnd();

    UFUNCTION(BlueprintCallable, Category = "TBG |Match")
    bool IsAnyUnitMoving() const;

    // -------------------------------
    // GESTIONE UNITÀ
    /** Spawna una specifica unità sulla griglia */
    UFUNCTION(BlueprintCallable, Category = "TBG|Match")
    void SpawnUnit(TSubclassOf<ATBGBaseUnit> UnitClass, int32 InX, int32 InY, EUnitTurnOwner UnitOwner);

    /** Gestisce la morte e il successivo respawn dell'unità */
    UFUNCTION(BlueprintCallable, Category = "TBG|Units")
    void OnUnitDied(ATBGBaseUnit* Unit);

    // -------------------------------
    // LOGICA TORRI E VITTORIA
    /** Aggiorna lo stato di tutte le torri (Neutrale/Contesa/Controllata) */
    void EvaluateTowerStates();

    /** Conta le torri possedute */
    void TowerCounts();

    /** Verifica se qualcuno ha vinto (es: 2 torri per 2 turni) */
    void CheckVictoryCondition();

    // -------------------------------
    // QUERY DI STATO (Getters)
    /** Ritorna true se è il turno del giocatore umano */
    bool IsHumanTurn() const { return bIsHumanTurn; }
    EMatchPhase GetCurrentPhase() const { return CurrentPhase; }
    AGridMapManager* GetGridMapManager() const { return GridMapManager; }
    int32 GetCurrentTurnNumber() const { return CurrentTurnNumber; }


    bool bHumanPlacedBrawler = false;
    bool bHumanPlacedSniper = false;
    bool bAIPlacedBrawler = false;
    bool bAIPlacedSniper = false;
    int32 PlaceUnitHumanLeft = 2;
    int32 PlaceUnitAILeft = 2;

protected:

    UPROPERTY()
    ATBGGameStateBase* GameStateBasePP;

    // CLASSI BLUEPRINT
    UPROPERTY(EditAnywhere, Category = "TBG|Classes")
    TSubclassOf<ATBGGameStateBase> GameStateBase;

    UPROPERTY(EditAnywhere, Category = "TBG|Classes")
    TSubclassOf<ATBGAIController> AIControllerClass;

    UPROPERTY(EditAnywhere, Category = "TBG|Classes")
    TSubclassOf<ATBGUnitSniper> SniperClass;

    UPROPERTY(EditAnywhere, Category = "TBG|Classes")
    TSubclassOf<ATBGUnitBrawler> BrawlerClass;

    // La classe dell'unità attualmente selezionata dall'UI
    UPROPERTY(BlueprintReadWrite, Category = "Placement")
    TSubclassOf<ATBGBaseUnit> SelectedUnit;

private:
    // RIFERIMENTI E STATO INTERNO
    UPROPERTY()
    TObjectPtr<AGridMapManager> GridMapManager;

    UPROPERTY()
    TObjectPtr<ATBGPlayerController> PlayerController;

    UPROPERTY()
    TObjectPtr<ATBGAIController> AIController;

public:
    // Unità Player
    UPROPERTY(EditAnywhere, Category = "Player Setup")
    TSubclassOf<ATBGUnitSniper> PlayerSniper;
    UPROPERTY(EditAnywhere, Category = "Player Setup")
    TSubclassOf<ATBGUnitBrawler> PlayerBrawler;

    // Unità AI
    UPROPERTY(EditAnywhere, Category = "AI Setup")
    TSubclassOf<ATBGUnitSniper> AISniper;

    UPROPERTY(EditAnywhere, Category = "AI Setup")
    TSubclassOf<ATBGUnitBrawler> AIBrawler;

    // Lista Torri
    UPROPERTY()
    TArray<TObjectPtr<ATBGTower>> Towers;

    // VARIABILI DI FLUSSO
    EMatchPhase CurrentPhase = EMatchPhase::WaitingForConfig;
    EPlacementSlot CurrentPlacementSlot = EPlacementSlot::FirstPlayer_First;
    EUnitTurnOwner CoinTossWinner;
    EUnitTurnOwner FirstPlayerOfTurn;

    // True se il giocatore umano ha vinto il coin flip
    bool bHumanGoesFirst = false;
    bool bIsHumanTurn = false;
    int32 CurrentTurnNumber = 0;
    int32 UnitsActedThisTurn = 0;

    /** Posizioni di spawn iniziali: Key = Unità, Value = FIntPoint(X,Y) */
    UPROPERTY()
    TMap<TObjectPtr<ATBGBaseUnit>, FIntPoint> SpawnPositions;
};
