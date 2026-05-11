// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "TBGGameMode.h"
#include "TBGUserWidget.generated.h"


UCLASS()
class DURANTEDAVIDE_TBG_API UTBGUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdatePersistentStats(int32 CurrentTurn, int32 HumanTowersCaptured, int32 AITowersCaptured);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateSniperAI(int32 CurrentHP, int32 MaxHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateSniperHuman(int32 CurrentHP,int32 MaxHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateBrawlerAI(int32 CurrentHP,int32 MaxHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateBrawlerHuman(int32 CurrentHP,int32 MaxHP);


    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_ShowUnitStats(int32 CurrentHP,
        int32 MaxHP,
        int32 MovementRange,
        int32 AttackRange,
        int32 DamageMin,
        int32 DamageMax,
        bool bIsPreview);


    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_ShowCoinFlipResult(EUnitTurnOwner Winner);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_ShowPlacementUI(bool bShow);

    // Inizialmente nasconde/mostra il bottone (es. durante UnitPlacement)
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_SetEndTurnButtonVisibility(bool bIsVisible);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_ShowTurnNotificationAction(EMatchPhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_ShowVictoryScreen(EUnitTurnOwner Winner);
};

