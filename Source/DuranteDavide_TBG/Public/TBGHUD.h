// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TBGGameMode.h"
#include "TBGGameInstance.h"
#include "TBGHUD.generated.h"

class UTBGUserWidget;

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGHUD : public AHUD
{
	GENERATED_BODY()
	
public:
    void UpdateHUDManual(EMatchPhase CurrentPhase, EUnitTurnOwner TurnOwner, int32 CurrentTurn, int32 HumanTowersCaptured, int32 AITowersCaptured);

    void RefreshUnitsUI();

    void ShowUnitStats(
        int32 CurrentHP,
        int32 MaxHP,
        int32 MovementRange,
        int32 AttackRange,
        int32 DamageMin,
        int32 DamageMax,
        bool bIsPreview);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UTBGUserWidget> MainWidgetClass;

    // Riferimento all'istanza creata
    UPROPERTY()
    UTBGUserWidget* MainWidget;

    // Flag per assicurarsi che l'animazione/scritta del Coin Flip appaia una sola volta
    bool bCoinFlipUIExecuted = false;
private:
    EMatchPhase LastPhase = EMatchPhase::CoinFlip;
    EUnitTurnOwner LastTurnOwner = EUnitTurnOwner::Human;
    EPlacementSlot PlacementSlot = EPlacementSlot::Complete;
};
