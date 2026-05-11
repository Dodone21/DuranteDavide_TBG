// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGAIController.h"
#include "GridMapManager.h"
#include "TBGTower.h"
#include "TBGBaseUnit.h"
#include "TBGUnitSniper.h"
#include "TBGUnitBrawler.h"
#include "TBGGameStateBase.h"
#include "TBGGameMode.h"
#include "Kismet/GameplayStatics.h"

ATBGAIController::ATBGAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATBGAIController::BeginPlay()
{
	Super::BeginPlay();
	GridMapManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    if (!GridMapManager)
    {
        UE_LOG(LogTemp, Error, TEXT("ATBGAIController::BeginPlay — GridMapManager non trovato."));
    }
}

FAIObjective ATBGAIController::ChooseBestObjective(ATBGBaseUnit* SourceUnit)
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::ChooseBestObjective chiamato"));

    FAIObjective Result;
    if (!IsValid(SourceUnit))   return Result;

    ATBGGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState<ATBGGameStateBase>() : nullptr;
    const int32 AITowers = GS ? GS->CurrentTowerScore.GetAITowers() : 0;
    const int32 HumanTowers = GS ? GS->CurrentTowerScore.GetHumanTowers() : 0;

    ATBGBaseUnit* ClosestEnemy = FindClosestEnemy(SourceUnit);
    const bool bEnemyInRange = IsValid(ClosestEnemy) &&
        FMath::Abs(SourceUnit->GetGridPosition().X - ClosestEnemy->GetGridPosition().X) +
        FMath::Abs(SourceUnit->GetGridPosition().Y - ClosestEnemy->GetGridPosition().Y) <= SourceUnit->GetAttackRange();

    // 1) Se posso attaccare, attacco subito
    if (bEnemyInRange)
    {
        Result.Type = EAIObjectiveType::Enemy;
        Result.Enemy = ClosestEnemy;
        return Result;
    }

        // 2) Se sono dentro una torre e sono il defender assegnato, valuto se devo restare
    ATBGTower* CurrentTower = FindTowerCurrentlyBeingCaptured(SourceUnit);
    if (IsValid(CurrentTower))
    {
        if (!IsValid(CurrentTower->AssignedDefender) || CurrentTower->AssignedDefender->IsDead())
        {
            CurrentTower->AssignedDefender = SourceUnit;
        }

        const bool bIsAssignedDefender = (CurrentTower->AssignedDefender == SourceUnit);
        const bool bAIIsWinning = (GS->CurrentTowerScore.GetAITowers() >= 2);
        const bool bAIIsNotBehind = (GS->CurrentTowerScore.GetAITowers() >= GS->CurrentTowerScore.GetHumanTowers());

        if (bIsAssignedDefender && (bAIIsWinning || bAIIsNotBehind))
        {
            Result.Type = EAIObjectiveType::HoldTower;
            Result.Tower = CurrentTower;
            return Result;
        }
    }

    // 3) Se non sto già presidiando “bene”, cerco la torre più vicina
    ATBGTower* ClosestTower = FindClosestTargetTower(SourceUnit);
    if (IsValid(ClosestTower))
    {
        Result.Type = EAIObjectiveType::Tower;
        Result.Tower = ClosestTower;
        return Result;
    }

    // 4) Se non ci sono torri utili, inseguo il nemico
    if (IsValid(ClosestEnemy))
    {
        Result.Type = EAIObjectiveType::Enemy;
        Result.Enemy = ClosestEnemy;
    }
    return Result;
}

void ATBGAIController::ExecuteAIUnitTurn() 
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::ExecuteAIUnitTurn chiamato")); 
    ProcessableAIUnits.Empty();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(Actor);
        if (!IsValid(Unit))
        {   continue;   }

        if (Unit->GetTurnOwnerType() == EUnitTurnOwner::AI && Unit->GetCurrentHP() > 0 && !Unit->IsHidden())
        {
            ProcessableAIUnits.Add(Unit);
        }
    }

    // Inizia il processo della prima unità
    ProcessNextAIUnit();
}

bool ATBGAIController::ExecuteUnitTurn(ATBGBaseUnit* Unit)
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::ExecuteUnitTurn per %s"), *Unit->GetName());
    if (!IsValid(Unit) ||
        Unit->GetCurrentHP() <= 0 ||
        Unit->GetTurnOwnerType() != EUnitTurnOwner::AI)
    {   return false; }

    const FAIObjective Objective = ChooseBestObjective(Unit);
    UE_LOG(LogTemp, Warning,TEXT("AI Objective = %d"), (int32)Objective.Type);

    // Attacco sempre prioritario se l'obiettivo è un nemico
    if (Objective.Type == EAIObjectiveType::Enemy && IsValid(Objective.Enemy))  {
        if (TryAttack(Unit, Objective.Enemy))   {
            UE_LOG(LogTemp, Log, TEXT("AI: Attacco eseguito con successo."));
            return false;
        }
    }

    // Se devo tenere una torre, non mi muovo
    if (Objective.Type == EAIObjectiveType::HoldTower)  {
        UE_LOG(LogTemp, Log, TEXT("AI: presidio torre, nessun movimento."));
        return false;
    }

    // Se devo muovermi verso una torre o un nemico, lo faccio
    if (!Unit->bHasMoved)
    {
        FIntPoint TargetPos = FIntPoint(-1, -1);

        if (Objective.Type == EAIObjectiveType::Tower && Objective.Tower)
        {
            TargetPos = Objective.Tower->GetGridPosition();
        }
        else if (Objective.Type == EAIObjectiveType::Enemy && Objective.Enemy)
        {
            TargetPos = Objective.Enemy->GetGridPosition();
        }

        if (TargetPos != FIntPoint(-1, -1))
        {
            if (TryMoveTowardTarget(Unit, TargetPos))
            {
                return true;
            }

            UE_LOG(LogTemp, Error, TEXT("AI: TryMoveTowardTarget ha fallito per %s verso (%d,%d)"),
                *Unit->GetName(), TargetPos.X, TargetPos.Y);
        }
    }
    return false;
}

void ATBGAIController::ProcessNextAIUnit()
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::ProcessNextAIUnit chiamato"));
    ATBGGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ATBGGameMode>() : nullptr;
    if (!GM)
    {
        return;
    }
    // Se non ci sono più unità, finiamo il turno

    while (ProcessableAIUnits.Num() > 0)
    {
        ATBGBaseUnit* Candidate = ProcessableAIUnits[0];

        if (IsValid(Candidate) && Candidate->GetCurrentHP() > 0 && !Candidate->IsHidden())
        {
            break;
        }

        ProcessableAIUnits.RemoveAt(0);
    }

    if (ProcessableAIUnits.Num() == 0)
    {
        FTimerHandle T;
        GetWorldTimerManager().SetTimer(T, GM, &ATBGGameMode::OnTurnComplete, 1.0f, false);
        return;
    }

    // Prendiamo l'unità corrente (senza rimuoverla ancora dalla lista)
    CurrentAIUnit = ProcessableAIUnits[0];
    ProcessableAIUnits.RemoveAt(0);

    FTimerHandle TWaitToMove;
    GetWorldTimerManager().SetTimer(
        TWaitToMove,
        FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                if (!IsValid(CurrentAIUnit))
                {
                    // Se l'unita' e' sparita, passiamo oltre
                    OnCurrentAIUnitActionFinished();
                    return;
                }

                // Diamo l'ordine di muoversi (questo avvia VisibileActionMoving nell'unità)
                bool bWaitingForMovement = ExecuteUnitTurn(CurrentAIUnit);

                if (bWaitingForMovement)
                {
                    // Ci iscriviamo al delegato nel Tick della BaseUnit
                    CurrentAIUnit->OnMovementFinished.AddUniqueDynamic(this, &ATBGAIController::OnCurrentAIUnitActionFinished);
                    UE_LOG(LogTemp, Log, TEXT("AI: In attesa che l'unità finisca di camminare..."));
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("AI: Nessun movimento avviato, procedo alla prossima unita'"));
                    OnCurrentAIUnitActionFinished();
                }
        }), 1.0f, false);
}

void ATBGAIController::OnCurrentAIUnitActionFinished()
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::OnCurrentAIUnitActionFinished chiamato"));
    if (CurrentAIUnit)
    { 
        // 1. Rimuoviamo l'ascolto per il movimento appena concluso
        CurrentAIUnit->OnMovementFinished.RemoveAll(this);

        // 2. TENTATIVO DI ATTACCO POST-MOVIMENTO
        // Controlliamo se può attaccare (non ha ancora attaccato in questo turno)
        if (!CurrentAIUnit->bHasAttacked)
        {
            // Cerchiamo l'obiettivo migliore dalla nuova posizione
            FAIObjective Objective = ChooseBestObjective(CurrentAIUnit);

            if (Objective.Type == EAIObjectiveType::Enemy && IsValid(Objective.Enemy))
            {
                // TryAttack restituirà true se il nemico è nel range e l'attacco riesce
                if (TryAttack(CurrentAIUnit, Objective.Enemy))
                {
                    UE_LOG(LogTemp, Warning, TEXT("AI: %s attacca dopo essersi mosso!"), *CurrentAIUnit->GetName());

                    // Se vuoi che ci sia una piccola pausa visiva dopo l'attacco prima di passare alla prossima unità
                    FTimerHandle TPostAttack;
                    GetWorldTimerManager().SetTimer(TPostAttack, [this]() {
                        this->FinishUnitTurnAndProceed();
                        }, 0.8f, false); // Pausa di 0.8 secondi per far vedere l'attacco

                    return; // Usciamo: il timer chiamerà la prossima unità
                }
            }
        }
    }
    // Se non ha attaccato o non c'erano bersagli, procediamo normalmente
    FinishUnitTurnAndProceed();
}

// Funzione di supporto per pulire e passare oltre
void ATBGAIController::FinishUnitTurnAndProceed()
{
    CurrentAIUnit = nullptr;
    ProcessNextAIUnit();
}

void ATBGAIController::CheckUnitMovementFinished(ATBGBaseUnit* Unit)
{
    // Se l'unità sta ancora camminando, richiamiamo questa funzione tra poco
    if (Unit && Unit->IsMoving())
    {
        FTimerHandle TRecheck;
        GetWorldTimerManager().SetTimer(TRecheck, [this, Unit]() { CheckUnitMovementFinished(Unit); }, 0.1f, false);
    }
    else{
        // L'UNITÀ È ARRIVATA A DESTINAZIONE
        // 5. PULIZIA E PASSAGGIO ALLA PROSSIMA
        if (GridMapManager) GridMapManager->ClearHighlights();

        if (ProcessableAIUnits.Num() > 0)
        {   ProcessableAIUnits.RemoveAt(0); }

        // delay next unit
        FTimerHandle TNext;
        GetWorldTimerManager().SetTimer(TNext, this, &ATBGAIController::ProcessNextAIUnit, 0.5f, false);
    }
}

//Piazzamento unita' nella fase di setup
void ATBGAIController::StartPlacementThinking()
{
    //Fare tutto quello che ha fatto AIPlacementAction in GameMode
    // Simula un tempo di "riflessione" dell'AI (es. 1.5 secondi)
    UE_LOG(LogTemp, Warning, TEXT("AI: Ho ricevuto l'ordine di pensare al piazzamento..."));
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ATBGAIController::ExecutePlacement, 1.5f, false);
}

//Piazzamento unita' nella fase di setup
void ATBGAIController::ExecutePlacement()
{
    UE_LOG(LogTemp, Warning, TEXT("AI: ExecutePlacement avviato!"));
    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM || !GM->GetGridMapManager()) { UE_LOG(LogTemp, Error, TEXT("AI: GM non trovato")); return; }
    if (GM->CurrentPlacementSlot == EPlacementSlot::Complete)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIController::ExecutePlacement__ Siamo in EPlacementSlot::Complete, non fare nulla"));
        return; // Non fare nulla se la fase è finita
    }

    // 2. TROVA UNA CELLA VALIDA NELLE ULTIME 3 RIGHE (Y = 22, 23, 24)
    AGridMapManager* GridMap = GM->GetGridMapManager();
    int32 TargetX = 0;
    int32 TargetY = 22;
    bool bFoundValidCell = false;

    // Cerca una cella casuale finché non ne trova una valida (max 50 tentativi per sicurezza)
    for (int i = 0; i < 50; i++)
    {
        int32 RandomX = FMath::RandRange(0, 24); // Griglia 25x25
        int32 RandomY = FMath::RandRange(22, 24); // Zona AI

        // Controlla se la cella è calpestabile (Z > 0) e non è occupata da altre unità o torri
        if (GridMap->IsValidCell(RandomX, RandomY) && GridMap->IsWalkable(RandomX, RandomY))
        {
            TargetX = RandomX;
            TargetY = RandomY;
            bFoundValidCell = true;
            break;
        }
    }
    // 3. COMUNICA LA SCELTA ALLA GAMEMODE
    if (bFoundValidCell) {
    UE_LOG(LogTemp, Log, TEXT("AI: Cella Valida Trovata."));

    TSubclassOf<ATBGBaseUnit> UnitForPlacement;
    if (!GM->bAIPlacedBrawler && !GM->bAIPlacedSniper)
    {
        UnitForPlacement = FMath::RandBool()? GM->AIBrawler : GM->AISniper;
    }
    else if (!GM->bAIPlacedBrawler){    UnitForPlacement = GM->AIBrawler;   }
    else if (!GM->bAIPlacedSniper){ UnitForPlacement = GM->AISniper;   }

        if (UnitForPlacement)
        {   GM->TryUnitPlacement(TargetX, TargetY, UnitForPlacement);   }
        else
        {   UE_LOG(LogTemp, Error, TEXT("AI: Errore! UnitForPlacement è NULL. Controlla le assegnazioni nel GameMode BP."));    }
    }
    else {
    UE_LOG(LogTemp, Error, TEXT("AI: Impossibile trovare una cella valida dopo 50 tentativi!"));
    }
}

bool ATBGAIController::TryMove(ATBGBaseUnit* Unit)
{
    if (!IsValid(Unit) || Unit->bHasMoved || !GridMapManager)
    {   return false;   }

    if (ATBGTower* TargetTower = FindClosestTargetTower(Unit))
    {
        return TryMoveTowardTarget(Unit, TargetTower->GetGridPosition());
    }

    if (ATBGBaseUnit* TargetEnemy = FindClosestEnemy(Unit))
    {
        return TryMoveTowardTarget(Unit, TargetEnemy->GetGridPosition());
    }

    return false;
}

bool ATBGAIController::TryMoveTowardTarget(ATBGBaseUnit* Unit, const FIntPoint& TargetPos)
{
    if (!IsValid(Unit) || Unit->bHasMoved || !GridMapManager)
    {   return false;   }

    const FIntPoint StartPos = Unit->GetGridPosition();

    // 2. Calcolo del punto di arrivo ottimale (usando la tua funzione)
    // Cerchiamo prima di arrivare adiacenti al target
    TArray<FIntPoint> AdjacentCells = {
        {TargetPos.X + 1, TargetPos.Y}, {TargetPos.X - 1, TargetPos.Y},
        {TargetPos.X, TargetPos.Y + 1}, {TargetPos.X, TargetPos.Y - 1}
    };

    FIntPoint GoalPos = FIntPoint(-1, -1);
    int32 BestDist = MAX_int32;
    for (const FIntPoint& Cell : AdjacentCells)
    {
        if (GridMapManager->IsValidCell(Cell.X, Cell.Y) && GridMapManager->IsWalkable(Cell.X, Cell.Y))
        {
            int32 Dist = FMath::Abs(StartPos.X - Cell.X) + FMath::Abs(StartPos.Y - Cell.Y);
            if (Dist < BestDist) { BestDist = Dist; GoalPos = Cell; }
        }
    }

    if (GoalPos == FIntPoint(-1, -1)) GoalPos = TargetPos;

    // 3. Otteniamo la destinazione finale rispettando il budget
    FIntPoint FinalDestination = GetFurthestReachableCellToward(Unit, StartPos, GoalPos);

    if (FinalDestination == FIntPoint(-1, -1) || FinalDestination == StartPos) return false;
    UE_LOG(LogTemp, Warning, TEXT("AI Debug: StartPos(%d,%d) - FinalDest(%d,%d)"), StartPos.X, StartPos.Y, FinalDestination.X, FinalDestination.Y);
    // 4. RECUPERO DEL PERCORSO VISIVO
        // Poiché MoveAlongPath ha bisogno dell'array di punti, dobbiamo richiedere 
        // il percorso dal GridMapManager per l'ultima volta verso la FinalDestination
    int32 UnusedCost = 0;
    TArray<FIntPoint> MyVisualPath = GridMapManager->GetPath(StartPos, FinalDestination, UnusedCost);
    if (MyVisualPath.Num() <= 1)
    {   return false;   }

    // Avvia l'animazione fluida
    Unit->VisibileActionMoving(MyVisualPath);

    // Aggiorna griglia logica e posizione fisica
    // 5. AGGIORNAMENTO LOGICO E FISICO
    //GridMapManager->SetUnit(StartPos.X, StartPos.Y, nullptr);
    GridMapManager->ClearUnit(StartPos.X, StartPos.Y);

    Unit->SetGridPosition(FinalDestination.X, FinalDestination.Y);
    GridMapManager->SetUnit(FinalDestination.X, FinalDestination.Y, Unit);

    // Controlla conquista torri dopo il movimento
    //CheckTowerCapture(FinalDestination);
    Unit->bHasMoved = true;
    return true;

}

bool ATBGAIController::TryAttack(ATBGBaseUnit* Unit, ATBGBaseUnit* TargetEnemy)
{
    if (!IsValid(Unit) ||
        !IsValid(TargetEnemy) ||
        Unit->bHasAttacked ||
        !GridMapManager)
    {
        return false;
    }

    // Sicurezza: il target deve essere realmente un nemico
    if (!IsEnemy(Unit, TargetEnemy))
    {
        return false;
    }

    const FIntPoint UnitPos = Unit->GetGridPosition();
    const FIntPoint EnemyPos = TargetEnemy->GetGridPosition();

    const int32 Dist =
        FMath::Abs(UnitPos.X - EnemyPos.X) +
        FMath::Abs(UnitPos.Y - EnemyPos.Y);


    if (Dist > Unit->GetAttackRange())
    {
        UE_LOG(LogTemp, Warning, TEXT("AIController Bersaglio fuori portata."));
        return false;
    }

    GridMapManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    // Spec: si può attaccare solo unità allo stesso livello o inferiore
    const int32 AttackerElevation = static_cast<int32>(
        GridMapManager->GetElevationLevel(UnitPos.X, UnitPos.Y));

    const int32 TargetElevation = static_cast<int32>(
        GridMapManager->GetElevationLevel(EnemyPos.X, EnemyPos.Y));

    if (AttackerElevation < TargetElevation)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIController: bersaglio troppo in alto."));
        return false;
    }

    const int32 Damage = FMath::RandRange(
        Unit->GetDamageMin(),
        Unit->GetDamageMax());

    UE_LOG(LogTemp, Warning,
        TEXT("AIController: %s attacca %s per %d danni."),
        *Unit->GetName(),
        *TargetEnemy->GetName(),
        Damage);

    // Applicazione danno
    TargetEnemy->ApplyTakenDamage(Damage);

    // Flag turno
    Unit->bHasAttacked = true;
    return true;
}

// ============================================================
//  VALUTAZIONE OBIETTIVI
// ============================================================

ATBGTower* ATBGAIController::FindClosestTargetTower(ATBGBaseUnit* SourceUnit)
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::FindClosestTargetTower chiamato."));
    if (!IsValid(SourceUnit))
    {   return nullptr; }

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGTower::StaticClass(), Found);

    ATBGTower* Best = nullptr;
    int32      MinDist = MAX_int32;

    const FIntPoint UnitPos = SourceUnit->GetGridPosition();

    for (AActor* A : Found)
    {
        ATBGTower* Tower = Cast<ATBGTower>(A);
        if (!IsValid(Tower))
        {
            continue;
        }

        // Se la torre è già controllata dall'AI, la ignoro
        if (Tower->GetController() == EUnitTurnOwner::AI)
        {
            continue;
        }

        const FIntPoint TowerPos = Tower->GetGridPosition();
        const int32 Dist = FMath::Abs(UnitPos.X - TowerPos.X) + FMath::Abs(UnitPos.Y - TowerPos.Y);

        if (Dist < MinDist)
        {
            MinDist = Dist;
            Best = Tower;
        }
    }
    return Best;
}

ATBGTower* ATBGAIController::FindTowerCurrentlyBeingCaptured(ATBGBaseUnit* Unit)
{
    if (!IsValid(Unit)) return nullptr;

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGTower::StaticClass(), Found);

    for (AActor* A : Found)
    {
        ATBGTower* Tower = Cast<ATBGTower>(A);

        if (!IsValid(Tower)) continue;

        if (Tower->IsUnitInsideCaptureRange(Unit))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("AI: %s sta gia' conquistando la torre %s"), *Unit->GetName(), *Tower->GetName());

            return Tower;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------

ATBGBaseUnit* ATBGAIController::FindClosestEnemy(ATBGBaseUnit* SourceUnit)
{
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::FindClosestEnemy chiamato."));
    if (!IsValid(SourceUnit))
    {   return nullptr; }

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), Found);

    ATBGBaseUnit* Best = nullptr;
    int32         MinDist = MAX_int32;

    const FIntPoint UnitPos = SourceUnit->GetGridPosition();

    for (AActor* A : Found)
    {
        ATBGBaseUnit* Other = Cast<ATBGBaseUnit>(A);
        if (!IsValid(Other))
        {   continue;   }

        if (!IsEnemy(SourceUnit, Other))
        {   continue;   }

        if (Other->GetCurrentHP() <= 0 || Other->IsHidden())
        {   continue;   }


        const FIntPoint OtherPos = Other->GetGridPosition();
        const int32 Dist = FMath::Abs(UnitPos.X - OtherPos.X) + FMath::Abs(UnitPos.Y - OtherPos.Y);

        if (Dist < MinDist)
        {
            MinDist = Dist;
            Best = Other;
        }
    }
    return Best;
}

FIntPoint ATBGAIController::GetFurthestReachableCellToward(ATBGBaseUnit* Unit, FIntPoint Start, FIntPoint Goal)
{
    // Validazione iniziale
    if (!IsValid(Unit) || !GridMapManager)
    {
        return FIntPoint(-1, -1);
    }

    int32 TotalPathCost = 0;
    // Usiamo il GridMapManager per ottenere il percorso e il costo totale
    TArray<FIntPoint> Path = GridMapManager->GetPath(Start, Goal, TotalPathCost);

    // Se non c'è un percorso o siamo già a destinazione
    if (Path.Num() <= 1) return FIntPoint(-1, -1);

    int32 MovBudget = Unit->GetMovementRange();
    UE_LOG(LogTemp, Log, TEXT("ATBGAIController::GetFurthestReachableCellToward: Unita %s ha Budget Movimento: %d"), *Unit->GetName(), MovBudget);

    // OTTIMIZZAZIONE: Se il costo totale rientra nel budget, 
    // restituiamo direttamente l'ultima cella senza fare cicli.
    if (TotalPathCost <= MovBudget)
    {   return Path.Last(); }

    // Se il costo totale supera il budget, dobbiamo tagliare il percorso.
    // Usiamo il ciclo per trovare l'ultima cella che possiamo permetterci.
    int32 FinalPathIdx = 0;
    int32 CurrentCost = 0;
    for (int32 i = 1; i < Path.Num(); i++)
    {
        int32 StepCost = GridMapManager->CalculateStepCost(Path[i - 1], Path[i]);

        if (CurrentCost + StepCost <= MovBudget)
        {
            CurrentCost += StepCost;
            FinalPathIdx = i;
        }
        else
        {
            // Budget esaurito, ci fermiamo alla cella precedente
            break;
        }
    }

    return Path[FinalPathIdx];
}

bool ATBGAIController::IsEnemy(ATBGBaseUnit* FirstUnit, ATBGBaseUnit* SecondUnit) const
{
    return FirstUnit && SecondUnit && FirstUnit->TurnOwnerType != SecondUnit->TurnOwnerType;
}