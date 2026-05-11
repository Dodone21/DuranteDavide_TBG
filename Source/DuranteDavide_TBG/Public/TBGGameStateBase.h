// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TBGGameInstance.h"
#include "TBGBaseUnit.h"
#include "TBGGameStateBase.generated.h"


// ============================================================
//  FPlayerTowerScore
//  Tiene traccia del controllo torri per un singolo giocatore.
//  Usata da GameMode per determinare la condizione di vittoria
//  (2 torri su 3 per 2 turni consecutivi).
// ============================================================
USTRUCT(BlueprintType)
struct FTowerScore
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towers")
    int32 HumanTowersControlledCounter = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Towers")
    int32 AITowersControlledCounter = 0;

    /** Contatori per la condizione di vittoria (2 torri per 2 turni) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "4Victory")
    int32 Human2Tower2Turns = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "4Victory")
    int32 AI2Tower2Turns = 0;

    int32 GetHumanTowers() const { return HumanTowersControlledCounter; }
    int32 GetAITowers() const { return AITowersControlledCounter; }
    //Chiamato da GameMode
    bool HumanVictory() const { return Human2Tower2Turns >= 4; }
    //Chiamato da GameMode
    bool AIVictory() const { return AI2Tower2Turns >= 4; }

    FTowerScore() {}
};

// ============================================================
//  FUnitStatus
//  Snapshot dello stato di un'unità per l'HUD.
//  GameMode aggiorna questi dati a ogni fine turno.
//  HUD li legge senza dover interrogare ogni ATBGBaseUnit.
// ============================================================
USTRUCT(BlueprintType)
struct FUnitStats
{
    GENERATED_BODY()

    // Identificativo testuale ("HP Sniper", "AI Brawler", ecc.)
    UPROPERTY(BlueprintReadOnly)
    FString DisplayName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentHP = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxHP = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AttackRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MovementRange = 0;

    // True se l'unità è attualmente nascosta (morta, in attesa di respawn)
    UPROPERTY(BlueprintReadOnly)
    bool bIsDead = false;
};


UCLASS()
class DURANTEDAVIDE_TBG_API ATBGGameStateBase : public AGameStateBase
{
	GENERATED_BODY() 
	
public:
	ATBGGameStateBase();

protected:

    virtual void BeginPlay() override;

public:
    // ----------------------------------------------------------
    // DATI DI TURNO
    // ----------------------------------------------------------
    // Numero di turno corrente (incrementato da GameMode)
    /** Numero del turno globale (0 Turno = Lancio moneta. 1 Turno = Turno Player + Turno Avversario) */
    UPROPERTY(VisibleAnywhere, Category = "Turn")
    int32 CurrentTurnNumber = 0;

    // A chi appartiene il turno corrente
    UPROPERTY(VisibleAnywhere, Category = "Turn")
    EUnitTurnOwner CurrentTurnOwner = EUnitTurnOwner::None;

    UPROPERTY()
    FTowerScore CurrentTowerScore;

    // ----------------------------------------------------------
    // GESTIONE TURNO
    // ----------------------------------------------------------
    /**
    * Aggiorna il numero di turno corrente e a chi appartiene.
    * Chiamato da GameMode all'inizio di ogni turno.
    */
    UFUNCTION(BlueprintCallable, Category = "Turn")
    void SetCurrentTurn(EUnitTurnOwner TurnOwner);

    /** Ritorna il numero di turno corrente */
    UFUNCTION(BlueprintPure, Category = "Turn")
    int32 GetCurrentTurnNumber() const { return CurrentTurnNumber; }

    /** Ritorna a chi appartiene il turno corrente */
    UFUNCTION(BlueprintPure, Category = "Turn")
    EUnitTurnOwner GetCurrentTurnOwner() const { return CurrentTurnOwner; }

    /** Incrementa il numero del turno (Chiamato solo dopo che entrambi hanno mosso) */
    //Chiamato da GameMode
    void IncrementTurn() { CurrentTurnNumber++; }

    /** Aggiorna il numero di torri possedute */
    //Chiamato da GameMode
    void SetTowerCounts(int32 NumberHumanTowers, int32 NumberAITowers);

    /** Valuta il progresso verso la vittoria basato sulle torri attuali */
    //Chiamato da GameMode
    void TowerVictoryProgress();
};
