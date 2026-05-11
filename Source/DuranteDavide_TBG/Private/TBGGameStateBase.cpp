// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGGameStateBase.h"
#include "TBGGameInstance.h"
#include "TBGHUD.h"
#include "TBGPlayerController.h"
#include "TBGGameMode.h"
#include "TBGBaseUnit.h"

ATBGGameStateBase::ATBGGameStateBase()
{
    CurrentTurnOwner = EUnitTurnOwner::None;
    CurrentTurnNumber = 0;
}

void ATBGGameStateBase::BeginPlay()
{
    Super::BeginPlay();
}

void ATBGGameStateBase::SetCurrentTurn(EUnitTurnOwner TurnOwner)
{
    CurrentTurnOwner = TurnOwner;

    UE_LOG(LogTemp, Log,
        TEXT("ATBGGameStateBase::SetCurrentTurn Owner: %s"),
        TurnOwner == EUnitTurnOwner::Human ? TEXT("Player") : TEXT("AI"));

    ATBGPlayerController* PlayerController = Cast<ATBGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM || !PlayerController) return;

    ATBGHUD* MyHUD = Cast<ATBGHUD>(PlayerController->GetHUD());
    if (MyHUD)
    {
        MyHUD->UpdateHUDManual(GM->GetCurrentPhase(), TurnOwner, GetCurrentTurnNumber(), CurrentTowerScore.GetHumanTowers(), CurrentTowerScore.GetAITowers());
    }

}

void ATBGGameStateBase::SetTowerCounts(int32 NumberHumanTowers, int32 NumberAITowers)
{
    CurrentTowerScore.HumanTowersControlledCounter = NumberHumanTowers;
    CurrentTowerScore.AITowersControlledCounter = NumberAITowers;
}

void ATBGGameStateBase::TowerVictoryProgress()
{
    // Verifica controllo Umano (>= 2 torri su 3)
    if (CurrentTowerScore.HumanTowersControlledCounter >= 2)
    {
        CurrentTowerScore.Human2Tower2Turns++;
    }
    else
    {
        CurrentTowerScore.Human2Tower2Turns = 0;
    }

    // Verifica controllo AI (>= 2 torri su 3)
    if (CurrentTowerScore.AITowersControlledCounter >= 2)
    {
        CurrentTowerScore.AI2Tower2Turns++;
    }
    else
    {
        CurrentTowerScore.AI2Tower2Turns = 0;
    }
    UE_LOG(LogTemp, Log, TEXT("Progresso Vittoria - Umano: %d/2 | AI: %d/2"), CurrentTowerScore.Human2Tower2Turns, CurrentTowerScore.AI2Tower2Turns);
}
