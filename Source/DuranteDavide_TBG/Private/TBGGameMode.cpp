// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGGameMode.h"
#include "GridMapManager.h"
#include "TBGTower.h"
#include "TBGGameStateBase.h"
#include "TBGBaseUnit.h"
#include "TBGUnitSniper.h"
#include "TBGUnitBrawler.h"
#include "TBGHUD.h"
#include "TBGGameInstance.h"
#include "TBGPlayerController.h"
#include "TBGAIController.h"
#include "Kismet/GameplayStatics.h"


ATBGGameMode::ATBGGameMode()
{
    CurrentPhase = EMatchPhase::WaitingForConfig;
    CurrentPlacementSlot = EPlacementSlot::FirstPlayer_First;
    bHumanGoesFirst = false;
    bIsHumanTurn = false;
    CurrentTurnNumber = 0;
    UnitsActedThisTurn = 0;
}

void ATBGGameMode::BeginPlay()
{
    Super::BeginPlay();
    PlayerController = Cast<ATBGPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AIController = GetWorld()->SpawnActor<ATBGAIController>(
        ATBGAIController::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams);

    InitMatch();
}

void ATBGGameMode::InitMatch()
{
    UE_LOG(LogTemp, Log, TEXT("ATBGGameMode_InitMatch - Avvio partita."));

    UTBGGameInstance* GI = Cast<UTBGGameInstance>(GetWorld()->GetGameInstance());
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("ATBGGameMode_InitMatch - GameInstance non trovata."));
        return;
    }

    // cercarlo nel livello
    GridMapManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    // Se NON lo ha trovato, allora crealo (Spawn)
    if (!GridMapManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridMapManager non trovato nel livello, procedo allo spawn..."));

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        GridMapManager = GetWorld()->SpawnActor<AGridMapManager>(AGridMapManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    if (GridMapManager)
    {
        GridMapManager->GenerateGrid();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("ATBGGameMode_InitMatch Impossibile trovare o spawnare GridMapManager!"));
    }

    // Raccoglie i riferimenti alle torri spawnate dalla griglia
    TArray<AActor*> FoundTowers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGTower::StaticClass(), FoundTowers);
    for (AActor* Actor : FoundTowers)
    {
        if (ATBGTower* Tower = Cast<ATBGTower>(Actor))
        {
            Towers.Add(Tower);
        }
    }

    // --- Avanza alla fase coin flip ---
    StartCoinFlip();

}
 
// bHumanGoesFirst è il booleano che memorizza chi comincia per primo
void ATBGGameMode::StartCoinFlip()
{
    CurrentPhase = EMatchPhase::CoinFlip;
    // Coin flip casuale
    bool StartingPlayer = (FMath::RandBool());
    CoinTossWinner = StartingPlayer ? EUnitTurnOwner::Human : EUnitTurnOwner::AI;
    if(CoinTossWinner == EUnitTurnOwner::Human) { bHumanGoesFirst = true; }else{ bHumanGoesFirst = false; }

    UE_LOG(LogTemp, Log,
        TEXT("ATBGGameMode_StartCoinFlip - %s inizia il piazzamento."),
        bHumanGoesFirst ? TEXT("Player") : TEXT("AI"));

    ATBGGameStateBase* GS = GetGameState<ATBGGameStateBase>();
    if (GS) {   GS->SetCurrentTurn(CoinTossWinner); }

    // Notifica HUD per mostrare il risultato del coin flip
    if (ATBGHUD* HUD = Cast<ATBGHUD>(UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetHUD()))
    {
        HUD->UpdateHUDManual(CurrentPhase, CoinTossWinner, 0, 0, 0);
    }
    FTimerHandle TimerHandle_PlacementDelay;
    GetWorldTimerManager().SetTimer(TimerHandle_PlacementDelay, this, &ATBGGameMode::StartPlacementPhase, 2.0f, false);
}

void ATBGGameMode::StartPlacementPhase()
{
    CurrentPhase = EMatchPhase::UnitPlacement;
    // Lo slot iniziale è SEMPRE FirstPlayer_First. 
    // Chi è il "FirstPlayer" lo sa la variabile bHumanGoesFirst.
    CurrentPlacementSlot = EPlacementSlot::FirstPlayer_First;
    UE_LOG(LogTemp, Log, TEXT("ATBGGameMode::StartPlacementPhase - Fase piazzamento unita'."));
    NotifyCurrentPlacementController();
}

void ATBGGameMode::NotifyCurrentPlacementController()
{
    ATBGGameStateBase* GS = GetGameState<ATBGGameStateBase>();
    if (!GS) return;

    // Chi ha vinto il Coinflip? (Il log dice che lo salvi in GetCurrentTurnOwner all'inizio)
    EUnitTurnOwner CoinflipWinner = CoinTossWinner;

    // Se siamo nello slot 1 o 3, tocca a chi ha VINTO il Coinflip.
    bool bIsFirstPlayerSlot = (CurrentPlacementSlot == EPlacementSlot::FirstPlayer_First ||
        CurrentPlacementSlot == EPlacementSlot::FirstPlayer_Second);

    // Calcoliamo di chi è il turno attuale
    EUnitTurnOwner ActiveOwner = bIsFirstPlayerSlot ? CoinflipWinner :
        (CoinflipWinner == EUnitTurnOwner::Human ? EUnitTurnOwner::AI : EUnitTurnOwner::Human);

    if (GS) { GS->SetCurrentTurn(ActiveOwner); }

    if (ActiveOwner == EUnitTurnOwner::Human)
    {
        // Tocca all'Umano
        UE_LOG(LogTemp, Warning, TEXT("GAMEMODE: Notifico il Player Umano per il piazzamento."));
        if (PlayerController)
        {
            PlayerController->Client_OnPlacementTurnStarted();
        }
    }
    else if(ActiveOwner == EUnitTurnOwner::AI)
    {
        // Tocca all'AI. Usiamo un piccolo ritardo (1.5 secondi) per far capire al giocatore cosa succede!
        UE_LOG(LogTemp, Warning, TEXT("GAMEMODE: Notifico l'AI per il piazzamento (con ritardo)."));
        // 1. Prova a cercarlo se esiste già
        if (!AIController) {
            AIController = Cast<ATBGAIController>(UGameplayStatics::GetActorOfClass(GetWorld(), ATBGAIController::StaticClass()));  }

        // 2. Se ancora non esiste, lo spawno usando la classe del Blueprint
        if (!AIController) {
            UE_LOG(LogTemp, Warning, TEXT("GameMode: Nessun ATBGAIController trovato, lo spawno ora..."));

            UClass* ClassToSpawn = AIControllerClass ? *AIControllerClass : ATBGAIController::StaticClass();
            AIController = GetWorld()->SpawnActor<ATBGAIController>(ClassToSpawn);
        }

        // 3. ORA CHE È SICURAMENTE VALIDO, LO ATTIVO!
        if (AIController) {
            UE_LOG(LogTemp, Log, TEXT("GameMode: AIController pronto, avvio ExecutePlacement..."));
            AIController->StartPlacementThinking();
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("GameMode: ERRORE CRITICO - Impossibile creare AIController!"));
        }
    }
}

void ATBGGameMode::TryUnitPlacement(int32 X, int32 Y, TSubclassOf<ATBGBaseUnit> UnitClassToSpawn)
{
    UE_LOG(LogTemp, Log, TEXT("GameMode TryUnitPlacement: X=%d Y=%d con l'Unita'=%s"), X, Y, UnitClassToSpawn ? *UnitClassToSpawn->GetName() : TEXT("NULL"));
    if (!GameStateBasePP) { GameStateBasePP = GetGameState<ATBGGameStateBase>(); }
    if (!GameStateBasePP || !GridMapManager || GetCurrentPhase() != EMatchPhase::UnitPlacement || GridMapManager->GetUnit(X, Y) != nullptr) return;
    
    if (!GridMapManager->IsWalkable(X, Y))
    {
        UE_LOG(LogTemp, Warning, TEXT("Piazzamento fallito: La cella (%d, %d) non è calpestabile (Acqua, Torre o già occupata)!"), X, Y);
        return;
    }
    
    EUnitTurnOwner CurrentPlacer = GameStateBasePP->GetCurrentTurnOwner();
    if (CurrentPlacer == EUnitTurnOwner::Human) {
        if (UnitClassToSpawn == PlayerBrawler && bHumanPlacedBrawler)
        {
            UE_LOG(LogTemp, Warning, TEXT("Il Brawler umano è già stato piazzato."));
            return;
        }
        if (UnitClassToSpawn == PlayerSniper && bHumanPlacedSniper)
        {
            UE_LOG(LogTemp, Warning, TEXT("Lo Sniper umano è già stato piazzato."));
            return;
        }
        if (Y > 2) 
        {
            UE_LOG(LogTemp, Warning, TEXT("Piazzamento fallito: Turno Umano ma Y > 2"));
            return;
        }
    }

    if (CurrentPlacer == EUnitTurnOwner::AI){
        if (UnitClassToSpawn == AIBrawler && bAIPlacedBrawler)
        {
            UE_LOG(LogTemp, Warning, TEXT("Il Brawler AI è già stato piazzato."));
            return;
        }

        if (UnitClassToSpawn == AISniper && bAIPlacedSniper)
        {
            UE_LOG(LogTemp, Warning, TEXT("Lo Sniper AI è già stato piazzato."));
            return;
        }

        if (Y < 22)
        {
            UE_LOG(LogTemp, Warning, TEXT("Piazzamento fallito: Turno AI ma Y < 22"));
            return;
        }
    }

    SpawnUnit(UnitClassToSpawn, X, Y, CurrentPlacer);
    if (CurrentPlacer == EUnitTurnOwner::Human){
        if (UnitClassToSpawn == PlayerBrawler)  bHumanPlacedBrawler = true;
        if (UnitClassToSpawn == PlayerSniper)   bHumanPlacedSniper = true;
    }

    if (CurrentPlacer == EUnitTurnOwner::AI)
    {
        if (UnitClassToSpawn == AIBrawler)  bAIPlacedBrawler = true;
        if (UnitClassToSpawn == AISniper)   bAIPlacedSniper = true;
    }
        
    (CurrentPlacer == EUnitTurnOwner::Human) ? PlaceUnitHumanLeft-- : PlaceUnitAILeft--;
    AdvancePlacementSlot();
}

void ATBGGameMode::SpawnUnit(TSubclassOf<ATBGBaseUnit> UnitClass, int32 InX, int32 InY, EUnitTurnOwner UnitOwner)
{
    if (!UnitClass || !GridMapManager) return;
    // Chiede la coordinata Z corretta al GridManager usando l'HeightMultiplier
    FVector Location = GridMapManager->GridToWorld(InX, InY);
    Location.Z += 100.0f; // Offset di sicurezza per lo spawn iniziale

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATBGBaseUnit* NewUnit = GetWorld()->SpawnActor<ATBGBaseUnit>(UnitClass, Location, FRotator::ZeroRotator, Params);

    if (NewUnit)
    {
        NewUnit->TurnOwnerType = UnitOwner;
        // Allinea l'unità alla superficie della tile bersaglio
        NewUnit->SetGridPosition(InX, InY);
        GridMapManager->SetUnit(InX, InY, NewUnit);
        PlayerController->SelectedUnitClassForPlacement = nullptr;

    }
}

void ATBGGameMode::StartTurnGame(EUnitTurnOwner TurnOwner, bool bIsStartOfMatch)
{
    if (!GameStateBasePP || !PlayerController) return;

    ATBGHUD* MyHUD = Cast<ATBGHUD>(PlayerController->GetHUD());
    int32 TurnNumber = GameStateBasePP->GetCurrentTurnNumber();
    int32 HUMTowerControll = GameStateBasePP->CurrentTowerScore.GetHumanTowers();
    int32 AITowerControll = GameStateBasePP->CurrentTowerScore.GetAITowers();

    if (GetCurrentPhase() == EMatchPhase::EndMatch || !GameStateBasePP)
    {
        if (MyHUD)MyHUD->UpdateHUDManual(CurrentPhase, TurnOwner, TurnNumber, HUMTowerControll, AITowerControll);
        return;
    }


    // Se la fase attuale NON è UnitPlacement, aggiorna la fase in base a chi appartiene il turno
    CurrentPhase = (TurnOwner == EUnitTurnOwner::Human) ? EMatchPhase::PlayerTurn : EMatchPhase::AITurn;

    // --- FASE 2: Assegnazione Turno ---
    if (GameStateBasePP)
    {   GameStateBasePP->SetCurrentTurn(TurnOwner); }

    // --- FASE 3: Gestione Unità (sostituisce i due cicli for separati) ---
    TArray<AActor*> Units;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), Units);

    for (AActor* A : Units)
    {
        ATBGBaseUnit* U = Cast<ATBGBaseUnit>(A);
        // Verifichiamo dinamicamente se l'unità appartiene a chi sta per giocare
        if (U && U->GetTurnOwnerType() == TurnOwner)
        {
            // Respawn se l'unità era morta
            if (U->IsDead())
            //if (U->IsHidden())
            {
                U->RespawnReset();
            }
            U->ResetTurnState();
        }
    }
    MyHUD->RefreshUnitsUI();

    // --- FASE 4: Logica Specifica AI ---
    if (TurnOwner == EUnitTurnOwner::AI && AIController)
    {
        FTimerHandle TimerHandle_AILogic;
        GetWorldTimerManager().SetTimer(TimerHandle_AILogic, [this]()
            {
                if (AIController)
                {
                    UE_LOG(LogTemp, Warning, TEXT("GameMode: Esecuzione turno AI."));
                    AIController->ExecuteAIUnitTurn();
                }
            }, 1.2f, false); // Un leggero ritardo per rendere l'azione visibile
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("GameMode: Turno Umano pronto. In attesa di input."));
    }
}

void ATBGGameMode::OnUnitActionComplete(ATBGBaseUnit* Unit)
{
    if (!Unit) return;
    UnitsActedThisTurn++;

    UE_LOG(LogTemp, Log,
        TEXT("ATBGGameMode::OnUnitActionComplete — %s ha completato le azioni (%d/2)."),
        *Unit->GetName(), UnitsActedThisTurn);

    // Entrambe le unità del giocatore corrente hanno agito
    if (UnitsActedThisTurn >= 2)
    {
        OnTurnComplete();
    }
}

void ATBGGameMode::OnTurnComplete()
{
    if (!GameStateBasePP) return;
    UE_LOG(LogTemp, Warning, TEXT("EndTurn chiamato da: %s. Turno Corrente: %d"),
        (GameStateBasePP->GetCurrentTurnOwner() == EUnitTurnOwner::Human ? TEXT("UMANO") : TEXT("AI")),
        GameStateBasePP->GetCurrentTurnNumber());

    // Valuta gli stati delle torri
    EvaluateTowerStates();
    TowerCounts();

    ATBGHUD* MyHUD = Cast<ATBGHUD>(PlayerController->GetHUD());
    if (MyHUD)MyHUD->RefreshUnitsUI();

    // Controlla condizione di vittoria
    EUnitTurnOwner CurrentOwner = GameStateBasePP->GetCurrentTurnOwner();
    EUnitTurnOwner NextOwner;
    if (CurrentOwner == CoinTossWinner)
    {
        // Ha appena mosso il primo giocatore del turno globale.
        // Passiamo la palla al secondo giocatore (non incrementiamo il TurnNumber).
        NextOwner = (CurrentOwner == EUnitTurnOwner::Human) ? EUnitTurnOwner::AI : EUnitTurnOwner::Human;
        UE_LOG(LogTemp, Warning, TEXT("Passaggio al secondo giocatore del turno..."));
    }
    else {
        GameStateBasePP->TowerVictoryProgress();
        CheckVictoryCondition();
        if (MatchWinner != EUnitTurnOwner::None)
        {
            if (MatchWinner == EUnitTurnOwner::AI) {
                EndMatch(MatchWinner);
                return;
            }
            else {
                EndMatch(MatchWinner);
                return;
            }
        }
        //nessuno ha vinto
        GameStateBasePP->IncrementTurn();
        NextOwner = CoinTossWinner;
        UE_LOG(LogTemp, Log, TEXT("Nuovo Round Globale iniziato."));

    }
    // 3. Unica chiamata per far partire il turno
    // Non chiamiamo SetCurrentTurn qui, lo farà StartTurnGame internamente.
    StartTurnGame(NextOwner, false);
}

void ATBGGameMode::EndMatch(EUnitTurnOwner Winner)
{
    CurrentPhase = EMatchPhase::EndMatch;

    // Log del vincitore usando l'enum
    FString WinnerName = (Winner == EUnitTurnOwner::Human) ? TEXT("Player") : TEXT("AI");
    UE_LOG(LogTemp, Log, TEXT("ATBGGameMode::EndMatch — Vince: %s."), *WinnerName);

    // Recupero dati dal GameState
    int32 TurnNumber = GameStateBasePP->GetCurrentTurnNumber();
    int32 HUMTowerControll = GameStateBasePP->CurrentTowerScore.GetHumanTowers();
    int32 AITowerControll = GameStateBasePP->CurrentTowerScore.GetAITowers();

    PlayerController = Cast<ATBGPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
    if (PlayerController)
    {
        // Aggiornamento HUD
        if (ATBGHUD* HUD = Cast<ATBGHUD>(PlayerController->GetHUD()))
        {
            // Passiamo direttamente 'Winner' che è già del tipo corretto
            HUD->UpdateHUDManual(CurrentPhase, Winner, TurnNumber, HUMTowerControll, AITowerControll);
        }

        // Impostazione Input Mode per la schermata finale
        FInputModeUIOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = true;
    }
}


void ATBGGameMode::EvaluateTowerStates()
{
    // Raccogli le unità una volta sola, fuori dal loop
    TArray<AActor*> AllUnitActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), AllUnitActors);

    // Filtra subito le unità valide per evitare cast ripetuti nel loop interno
    TArray<ATBGBaseUnit*> ActiveUnits;
    ActiveUnits.Reserve(AllUnitActors.Num());
    for (AActor* Actor : AllUnitActors)
    {
        ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(Actor);
        if (Unit && !Unit->IsHidden())
        {
            ActiveUnits.Add(Unit);
        }
    }

    //int32 PlayerTowers = 0;
    //int32 AITowers = 0;
    for (ATBGTower* Tower : Towers)
    {
        if (!Tower) continue;

        const int32 TowerX = Tower->GetGridX();
        const int32 TowerY = Tower->GetGridY();

        TArray<ATBGBaseUnit*> UnitsInZone;
        for (ATBGBaseUnit* Unit : ActiveUnits)
        {
            const int32 DistX = FMath::Abs(Unit->UnitGridPosition.X - TowerX);
            const int32 DistY = FMath::Abs(Unit->UnitGridPosition.Y - TowerY);
            if (FMath::Max(DistX, DistY) <= 2)
            {
                UnitsInZone.Add(Unit);
            }
        }

        Tower->EvaluateEEUpdateTowerControlState(UnitsInZone);
    }
}

void ATBGGameMode::TowerCounts()
{
    if (!GetWorld() || !GameStateBase) return;
    int32 HumanCount = 0;
    int32 AICount = 0;

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGTower::StaticClass(), Found);

    for (AActor* A : Found)
    {
        if (ATBGTower* Tower = Cast<ATBGTower>(A))
        {
            if (Tower->GetController() == EUnitTurnOwner::Human) HumanCount++;
            else if (Tower->GetController() == EUnitTurnOwner::AI) AICount++;
        }
    }

    ATBGGameStateBase* MyGameState = GetWorld()->GetGameState<ATBGGameStateBase>();
    // 2. Verifichiamo che il cast sia andato a buon fine (sicurezza)
    if (MyGameState)
    {
        // Ora puoi chiamare la funzione sull'istanza reale
        MyGameState->SetTowerCounts(HumanCount, AICount);
    }
}

//hciamato dal Pcontroller
void ATBGGameMode::CheckPlayerTurnEnd()
{
    // 1. Determina di chi è il turno attualmente
    EMatchPhase CurrentMatchPhase = GetCurrentPhase();
    EUnitTurnOwner CurrentOwner = (CurrentMatchPhase == EMatchPhase::PlayerTurn) ? EUnitTurnOwner::Human : EUnitTurnOwner::AI;

    // 2. Partiamo dal presupposto che tutte le unità abbiano finito
    bool bAllUnitsExhausted = true;

    // 3. Recupera tutte le unità (puoi usare UGameplayStatics::GetAllActorsOfClass
    // oppure una TArray che mantieni aggiornata nella GameMode)
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), FoundUnits);

    for (AActor* Actor : FoundUnits)
    {
        ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(Actor);

        // Controlliamo solo le unità vive del giocatore di turno
        if (Unit && Unit->GetTurnOwnerType() == CurrentOwner && !Unit->IsHidden())
        {
            // Se può muovere, il turno non è finito
            if (!Unit->bHasMoved)
            {
                // Se questa unità deve ancora muovere, il turno NON è finito.
                bAllUnitsExhausted = false;
                continue;// Passa alla prossima unità della lista
            }

            // Se ha mosso ma deve attaccare, controlliamo se può davvero farlo
            if (!Unit->bHasAttacked)
            {
                bool bCanActuallyAttack = false;
                // --- LOGICA INTEGRATA (Invece di chiamare la funzione esterna) ---
                for (AActor* PotentialTargetActor : FoundUnits) // Riutilizziamo la lista già recuperata
                {
                    ATBGBaseUnit* Target = Cast<ATBGBaseUnit>(PotentialTargetActor);
                    if (Target && Target->GetTurnOwnerType() != CurrentOwner && !Target->IsHidden())
                    {

                        int32 Dist = FMath::Abs(Unit->GetGridPosition().X - Target->GetGridPosition().X) +
                                     FMath::Abs(Unit->GetGridPosition().Y - Target->GetGridPosition().Y);
                        if (Dist <= Unit->GetAttackRange())
                        {
                            bCanActuallyAttack = true;
                            break; // Trovato un bersaglio, smetti di cercare per questa unità
                        }
                    }
                }

                if (bCanActuallyAttack) // Re-inserisci questo controllo!
                {
                    bAllUnitsExhausted = false;
                }
                else
                {
                    Unit->bHasAttacked = true; // Chiudi l'azione se non c'è nessuno
                    UE_LOG(LogTemp, Log, TEXT("Unità %s non ha bersagli, azione attacco chiusa."), *Unit->GetName());
                }
            }
        }
    }

    // 3. FINE TURNO: Scatta solo se TUTTE le unità hanno bHasMoved=true AND bHasAttacked=true
    // 4. Se dopo aver controllato tutte le unità, non ci sono più azioni disponibili:
    if (bAllUnitsExhausted)
    {
        if (IsAnyUnitMoving())
        {
            FTimerHandle TWait;
            GetWorldTimerManager().SetTimer(TWait, this, &ATBGGameMode::CheckPlayerTurnEnd, 0.2f, false);
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("Tutte le unità hanno esaurito le azioni. Fine turno automatica."));
        OnTurnComplete(); // Chiama la tua funzione pulita che abbiamo sistemato prima!
    }
}

bool ATBGGameMode::IsAnyUnitMoving() const
{
    // 1. Recuperiamo tutte le unità esistenti nella scena
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), FoundUnits);

    for (AActor* Actor : FoundUnits)
    {
        ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(Actor);

        // 2. Controlliamo se l'unità è valida e se si sta muovendo
        // Uso bIsMoving (o il nome della variabile che usi nel Tick della BaseUnit)
        if (Unit && Unit->bIsMoving)
        {
            UE_LOG(LogTemp, Warning, TEXT("ATBGGameMode::IsAnyUnitMoving "));
            // Se anche una sola unità si muove, restituiamo true immediatamente
            return true;
        }
    }

    // 3. Se il ciclo finisce, significa che sono tutte ferme
    return false;
}

void ATBGGameMode::CheckVictoryCondition()
{
    ATBGHUD* MyHUD = Cast<ATBGHUD>(PlayerController->GetHUD());
    ATBGGameStateBase* GS = GetGameState<ATBGGameStateBase>();
    if (!GS) return;

    // Spec: vince chi controlla >= 2 torri per 2 turni consecutivi
    if (GS->CurrentTowerScore.HumanVictory())
    {
        UE_LOG(LogTemp, Log, TEXT("ATBGGameMode::CheckVictoryCondition — Player ha vinto"));
        MatchWinner = EUnitTurnOwner::Human;
        return;
    }

    if (GS->CurrentTowerScore.AIVictory())
    {
        UE_LOG(LogTemp, Log, TEXT("ATBGGameMode::CheckVictoryCondition — AI ha vinto"));
        MatchWinner = EUnitTurnOwner::AI;
        return;
    }
    MatchWinner = EUnitTurnOwner::None;
    return;
}

void ATBGGameMode::OnUnitDied(ATBGBaseUnit* InUnit)
{
    if (!InUnit || !GridMapManager) return;
    UE_LOG(LogTemp, Log, TEXT("ATBGGameMode::OnUnitDied - %s eliminata."),*InUnit->GetName());
}

void ATBGGameMode::AdvancePlacementSlot()
{
    EUnitTurnOwner CoinflipWinner = CoinTossWinner;
    EUnitTurnOwner Opponent = (CoinflipWinner == EUnitTurnOwner::Human) ? EUnitTurnOwner::AI : EUnitTurnOwner::Human;
    // 1. Avanzamento logico
    switch (CurrentPlacementSlot)
    {
    case EPlacementSlot::FirstPlayer_First:
        CurrentPlacementSlot = EPlacementSlot::SecondPlayer_First;
        break;
    case EPlacementSlot::SecondPlayer_First:
        CurrentPlacementSlot = EPlacementSlot::FirstPlayer_Second;
        break;
    case EPlacementSlot::FirstPlayer_Second:
        CurrentPlacementSlot = EPlacementSlot::SecondPlayer_Second;
        break;
    case EPlacementSlot::SecondPlayer_Second:
        CurrentPlacementSlot = EPlacementSlot::Complete;
        break;
    default: break;
    }
    UE_LOG(LogTemp, Log, TEXT("Slot avanzato a: %d"), (int32)CurrentPlacementSlot);

    // 3. Controllo Fine
    if (CurrentPlacementSlot == EPlacementSlot::Complete)
    {
        UE_LOG(LogTemp, Warning, TEXT("Piazzamento completato. Avvio battaglia!"));

        // 1. Recupera tutte le unità presenti in campo
        TArray<AActor*> FoundUnits;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGBaseUnit::StaticClass(), FoundUnits);

        // 2. Cicla su ogni unità e fissa la sua posizione di partenza
        for (AActor* UnitActor : FoundUnits)
        {
            ATBGBaseUnit* Unit = Cast<ATBGBaseUnit>(UnitActor);
            if (Unit)
            {
                Unit->ConfirmStartingPosition();
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("Match Iniziato: Tutte le posizioni di partenza sono state salvate!"));
        PlayerController->SelectedUnitClassForPlacement = nullptr;
        StartTurnGame(CoinflipWinner, true);
        return; // CRITICO: Esci qui per non eseguire Notify al punto 5!
    }

    // 4. Aggiorniamo il Turno nel GameState in base allo slot attuale
    // Se lo slot è "FirstPlayer_...", il turno è del vincitore del coinflip
    bool bIsFirstPlayerSlot = (CurrentPlacementSlot == EPlacementSlot::FirstPlayer_First ||
                               CurrentPlacementSlot == EPlacementSlot::FirstPlayer_Second);

    EUnitTurnOwner NextTurnOwner = bIsFirstPlayerSlot ? CoinflipWinner : Opponent;
    if (GameStateBasePP)
    {
        GameStateBasePP->SetCurrentTurn(NextTurnOwner);
    }

    // 5. Notifichiamo i Controller
    NotifyCurrentPlacementController();
}
