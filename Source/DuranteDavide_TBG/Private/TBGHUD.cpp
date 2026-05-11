// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGHUD.h"
#include "TBGUserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"


void ATBGHUD::BeginPlay()
{
    Super::BeginPlay();

    if (MainWidgetClass)
    {
        MainWidget = CreateWidget<UTBGUserWidget>(GetWorld(), MainWidgetClass);
        if (MainWidget)
        {
            MainWidget->AddToViewport();
        }
    }
}

void ATBGHUD::ShowUnitStats(int32 CurrentHP,int32 MaxHP,int32 MovementRange,int32 AttackRange, int32 DamageMin, int32 DamageMax, bool bIsPreview)
{
    if (!MainWidget) return;
    MainWidget->BP_ShowUnitStats(CurrentHP,MaxHP,MovementRange,AttackRange,DamageMin,DamageMax,bIsPreview);
}

void ATBGHUD::RefreshUnitsUI()
{
    if (!MainWidget)return;
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), FoundUnits);

    for (AActor* Actor : FoundUnits)
    {
        ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(Actor);

        if (!Unit)continue;

        const bool bIsAI =Unit->GetTurnOwnerType() == EUnitTurnOwner::AI;
        const FString Name = Unit->GetName();
        //-----------------------------------
        // SNIPER AI
        //-----------------------------------
        if (Name.Contains(TEXT("Sniper")) && bIsAI)
        {
            MainWidget->BP_UpdateSniperAI(Unit->GetCurrentHP(),Unit->GetMaxHP());
        }
        //-----------------------------------
        // SNIPER HUMAN
        //-----------------------------------
        else if (Name.Contains(TEXT("Sniper")) && !bIsAI)
        {
            MainWidget->BP_UpdateSniperHuman(Unit->GetCurrentHP(),Unit->GetMaxHP());
        }
        //-----------------------------------
        // BRAWLER AI
        //-----------------------------------
        else if (Name.Contains(TEXT("Brawler")) && bIsAI)
        {
            MainWidget->BP_UpdateBrawlerAI(Unit->GetCurrentHP(),Unit->GetMaxHP());
        }
        //-----------------------------------
        // BRAWLER HUMAN
        //-----------------------------------
        else if (Name.Contains(TEXT("Brawler")) && !bIsAI)
        {
            MainWidget->BP_UpdateBrawlerHuman(Unit->GetCurrentHP(),Unit->GetMaxHP());
        }
    }
}

void ATBGHUD::UpdateHUDManual(EMatchPhase CurrentPhase, EUnitTurnOwner TurnOwner, int32 CurrentTurn, int32 HumanTowersCaptured, int32 AITowersCaptured)
{
    UE_LOG(LogTemp, Warning, TEXT("HUD Update: Phase = %d, Owner = %d"), (int32)CurrentPhase, (int32)TurnOwner);
    if (!MainWidget) return;
    // Aggiorna sempre le statistiche persistenti
    MainWidget->BP_UpdatePersistentStats(CurrentTurn, HumanTowersCaptured, AITowersCaptured);

    if (CurrentPhase == EMatchPhase::CoinFlip || CurrentPhase == EMatchPhase::UnitPlacement)
    {
        // Usiamo un flag per assicurarci di chiamarlo una sola volta all'inizio del match
        if (!bCoinFlipUIExecuted)
        {
            UE_LOG(LogTemp, Warning, TEXT("HUD: Tento di mostrare CoinFlip Result..."));
            MainWidget->BP_ShowCoinFlipResult(TurnOwner);
            bCoinFlipUIExecuted = true;
        }
        // Se siamo in CoinFlip, usciamo per non mostrare ancora la UI di piazzamento
        if (CurrentPhase == EMatchPhase::CoinFlip) return;
    }

    // 1. GESTIONE UNIT PLACEMENT
    if (CurrentPhase == EMatchPhase::UnitPlacement)
    {
        if (TurnOwner == EUnitTurnOwner::Human)
        {
            // Mostra i bottoni Sniper/Brawler (Index 1 dello Switcher)
            MainWidget->BP_ShowPlacementUI(true);
        }
        else
        {
            // Nascondi i bottoni (Index 0 dello Switcher)
            MainWidget->BP_ShowPlacementUI(false);
        }
    }

    if (CurrentPhase != LastPhase && PlacementSlot == EPlacementSlot::Complete)
    {
        MainWidget->BP_ShowPlacementUI(false);
    }

    // 2. GESTIONE SCRITTE DI INIZIO TURNO

    if (CurrentPhase != LastPhase)
    {
        if (CurrentPhase == EMatchPhase::PlayerTurn || CurrentPhase == EMatchPhase::AITurn)
        {
            // Passiamo semplicemente la fase, il BP deciderà cosa scrivere
            MainWidget->BP_ShowTurnNotificationAction(CurrentPhase);

            // La visibilità del bottone puoi lasciarla qui o spostarla nel BP
            bool bIsPlayerTurn = (CurrentPhase == EMatchPhase::PlayerTurn);
            MainWidget->BP_SetEndTurnButtonVisibility(bIsPlayerTurn);
        }
    }

    // 3. GESTIONE FINE PARTITA
    if (CurrentPhase == EMatchPhase::EndMatch)
    {
        if (CurrentPhase != LastPhase || TurnOwner != LastTurnOwner) 
        {
            if (MainWidget)
            {
                MainWidget->BP_SetEndTurnButtonVisibility(false);
                MainWidget->BP_ShowVictoryScreen(TurnOwner);
            }
        }
        LastPhase = CurrentPhase;
        LastTurnOwner = TurnOwner;
    }
    LastPhase = CurrentPhase;
    LastTurnOwner = TurnOwner;
}