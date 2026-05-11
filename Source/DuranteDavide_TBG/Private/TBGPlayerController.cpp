// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGPlayerController.h"
#include "GridMapManager.h"
#include "TBGTower.h"
#include "TBGGameMode.h"
#include "Tile.h"
#include "EnhancedInputComponent.h"
#include "TBGUserWidget.h"
#include "TBGHUD.h"
#include "EnhancedInputSubsystems.h"
#include "TBGUnitBrawler.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "TBGUnitSniper.h"
#include "DrawDebugHelpers.h"
#include "TBGBaseUnit.h"
#include "Kismet/GameplayStatics.h"

ATBGPlayerController::ATBGPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
    SelectedUnitClass = nullptr;
    Grid = nullptr;
}

void ATBGPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
}

void ATBGPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 1. Riferimento alla Griglia (Già presente)
    Grid = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    if (!Grid){ UE_LOG(LogTemp, Error, TEXT("ATBGPlayerController::BeginPlay - GridMapManager non trovato."));  }

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    bShowMouseCursor = true;

    // 2. CONFIGURAZIONE INPUT (Fondamentale per il Tick e il Mouse)
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);

    // 4. RESET VARIABILI DI STATO
    bIsPlacingUnit = false;
    SelectedUnitClass = nullptr;
}

void ATBGPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ATBGPlayerController::OnLeftClick);
    }
}

void ATBGPlayerController::OnPlacementTurnStarted()
{
    // 1. Sblocca la capacità del giocatore di fare input per il piazzamento
    SelectedUnitClass = nullptr; // Resetta l'unità selezionata
    

    UE_LOG(LogTemp, Log, TEXT("OnPlacementTurnStarted_ Turno piazzamento Umano iniziato. In attesa di input..."));
    if (ATBGHUD* TBGHUD = Cast<ATBGHUD>(GetHUD())) 
    {
        TBGHUD->UpdateHUDManual(EMatchPhase::UnitPlacement, EUnitTurnOwner::Human, 0, 0, 0);
    }
}

void ATBGPlayerController::Client_OnPlacementTurnStarted_Implementation()
{
    FInputModeGameAndUI InputMode;
    SetInputMode(InputMode);
    OnPlacementTurnStarted();
}

// ============================================================
//  INPUT HANDLERS
void ATBGPlayerController::OnLeftClick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    ATBGGameStateBase* GS = Cast<ATBGGameStateBase>(UGameplayStatics::GetGameState(GetWorld()));
    // Blocca input se non è il turno del Player
    if (!GS || GS->GetCurrentTurnOwner() != EUnitTurnOwner::Human) return;
    if (!GM) UE_LOG(LogTemp, Error, TEXT("GM è NULL nel click!"));

    AActor* HitActor = nullptr;
    if (GetInteractableUnderCursor(HitActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("HitActor: %s"), *GetNameSafe(HitActor));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Nessun actor sotto il cursore"));
    }

    // ==========================================
    // 1. ABBIAMO CLICCATO UNA CELLA (TILE)?
    // ==========================================
    if (ATile* ClickedTile = Cast<ATile>(HitActor))
    {
        int32 X = ClickedTile->GetGridX();
        int32 Y = ClickedTile->GetGridY();

        // PRIMA controlla se stai piazzando
        if (GM && GM->GetCurrentPhase() == EMatchPhase::UnitPlacement)
        {
            if (SelectedUnitClassForPlacement) 
            {
                UE_LOG(LogTemp, Warning, TEXT("Provo placement in (%d, %d)"), X, Y);
                GM->TryUnitPlacement(X, Y, SelectedUnitClassForPlacement);
            }
        }
        // --- FASE BATTAGLIA (Movimento) ---
        // Se arriviamo qui, o non siamo in UnitPlacement o non abbiamo nulla da piazzare
         // Nota: intendi l'istanza dell'unità che vuoi muovere
        if (SelectedUnitClass)
        {
            if (!SelectedUnitClass->bHasMoved)
            {
                MoveUnit(ClickedTile);
            }
        }
        return;
    }

    if (ATBGBaseUnit* ClickedUnit = Cast<ATBGBaseUnit>(HitActor))
    {
        // CASO A: È un'unità MIA
        if (ClickedUnit->GetTurnOwnerType() == EUnitTurnOwner::Human)
        {
            SelectUnit(ClickedUnit);
            ShowUnitStatsFromUnit(ClickedUnit);
            return;
        }
        // CASO B: È un'unità NEMICA
        if (SelectedUnitClass)// Ho un'unità mia selezionata?
        {
            if (!SelectedUnitClass->bHasAttacked)
            {
                UE_LOG(LogTemp, Warning, TEXT("Provo attacco da %s a %s"), *SelectedUnitClass->GetName(), *ClickedUnit->GetName());
                AttemptAttack(ClickedUnit);
                ShowUnitStatsFromUnit(ClickedUnit);
            }
            else {UE_LOG(LogTemp, Warning, TEXT("L'unità selezionata ha già attaccato!"));}
        }
        return; 
    }
    if (GM->GetCurrentPhase() == EMatchPhase::UnitPlacement)
    {
        UE_LOG(LogTemp, Warning, TEXT("SelectedUnitClassForPlacement: %s"),
            *GetNameSafe(SelectedUnitClassForPlacement));
        SelectedUnitClassForPlacement = nullptr;
    }
}

void ATBGPlayerController::OnRightClick()
{
    // Click destro deseleziona l'unità corrente
    if (SelectedUnitClass)
    {
        SelectUnit(nullptr);
    }
}

// ============================================================
//  LOGICA DI GIOCO
// ============================================================

// PER LA UI E L'HUD
void ATBGPlayerController::ShowPreviewStats()
{
    if (!SelectedUnitClassForPlacement) return;

    const ATBGBaseUnit* DefaultUnit =
        SelectedUnitClassForPlacement->GetDefaultObject<ATBGBaseUnit>();

    if (!DefaultUnit) return;

    ATBGHUD* HUD = Cast<ATBGHUD>(GetHUD());
    if (HUD)
    {
        HUD->ShowUnitStats(
            DefaultUnit->MaxHP,   // CurrentHP finto
            DefaultUnit->MaxHP,
            DefaultUnit->MovementRange,
            DefaultUnit->AttackRange,
            DefaultUnit->DamageMin,
            DefaultUnit->DamageMax,
            true);
    }
}

void ATBGPlayerController::ShowUnitStatsFromUnit(ATBGBaseUnit* Unit)
{
    if (!Unit) return;

    if (ATBGHUD* HUD = Cast<ATBGHUD>(GetHUD()))
    {
        HUD->ShowUnitStats(
            Unit->CurrentHP,
            Unit->MaxHP,
            Unit->MovementRange,
            Unit->AttackRange,
            Unit->DamageMin,
            Unit->DamageMax,
            false
        );
    }
}

//________________SELEZIONE DELLE UNITA__________
void ATBGPlayerController::SelectBrawlerForPlacement()
{

    ATBGGameMode* GM = GetWorld()->GetAuthGameMode<ATBGGameMode>();
    if (GM && GM->PlayerBrawler)
    {
        SelectedUnitType = EUnitType::Brawler;
        // Assegni la classe BP definita nel GameMode, non quella C++ "nuda"
        SelectedUnitClassForPlacement = GM->PlayerBrawler;
        UE_LOG(LogTemp, Warning, TEXT("Brawler selezionato correttamente dal GameMode"));
    }
    else
    {   UE_LOG(LogTemp, Error, TEXT("Errore: PlayerBrawler non settato nel GameMode!"));    }

    ShowPreviewStats();
}

void ATBGPlayerController::SelectSniperForPlacement()
{
    ATBGGameMode* GM = GetWorld()->GetAuthGameMode<ATBGGameMode>();
    if (GM && GM->PlayerSniper)
    {
        SelectedUnitType = EUnitType::Sniper;
        // Assegni la classe BP definita nel GameMode, non quella C++ "nuda"
        SelectedUnitClassForPlacement = GM->PlayerSniper;
        UE_LOG(LogTemp, Warning, TEXT("Sniper selezionato correttamente dal GameMode"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Errore: PlayerSniper non settato nel GameMode!"));
    }

    ShowPreviewStats();
}

void ATBGPlayerController::SelectUnit(ATBGBaseUnit* Unit)
{
    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return;


    // --- LOGICA DI GIOCO NORMALE ---
    if (Unit && Unit->TurnOwnerType == EUnitTurnOwner::Human) 
    {
        // Se c'era già un'unità selezionata, togliamo l'highlight alla vecchia cella
        if (SelectedUnitClass == Unit)
        {
            //SelectedUnitClass->ToggleHighlightFunct(false);
            //if (Grid) Grid->ClearHighlights();
            SelectedUnitClass = nullptr;
            return;
        }

        SelectedUnitClass = Unit;
        ShowUnitStatsFromUnit(Unit);
        UE_LOG(LogTemp, Log, TEXT("Unità selezionata per il turno: %s"), *Unit->GetName());
    }
}


void ATBGPlayerController::MoveUnit(ATile* Tile)
{
    // 1. Validazione iniziale
    if (!SelectedUnitClass || SelectedUnitClass->bHasMoved || !Grid || !Tile) return;

    FIntPoint StartPos = SelectedUnitClass->GetGridPosition();
    FIntPoint TargetPos = Tile->GetGridPosition();

    // 2. Calcolo percorso e costo totale tramite GridManager
    int32 TotalCost = 0;
    TArray<FIntPoint> Path = Grid->GetPath(StartPos, TargetPos, TotalCost);
    // Se non esiste un percorso valido
    if (Path.Num() <= 1) return;

    // 3. Verifica del budget di movimento
    if (TotalCost > SelectedUnitClass->GetMovementRange())
    {
        UE_LOG(LogTemp, Warning, TEXT("Movimento fallito: costo %d > range %d"), TotalCost, SelectedUnitClass->GetMovementRange());
        return;
    }
    else
    {
        // --- LOGICA DI MOVIMENTO FLUIDO ---
        // A. AVVIA IL MOVIMENTO VISIVO (Tick)
        // Passiamo l'intero array Path per far seguire le curve
        SelectedUnitClass->VisibileActionMoving(Path);

        // B. AGGIORNA POSIZIONE LOGICA
        // Svuota la vecchia cella
        //Grid->SetUnit(StartPos.X, StartPos.Y, nullptr);
        Grid->ClearUnit(StartPos.X, StartPos.Y);

        // Imposta le nuove coordinate GridX/Y (bIsMoving è già true, quindi salta il teletrasporto)
        SelectedUnitClass->SetGridPosition(TargetPos.X, TargetPos.Y);

        // Registra l'unità nella nuova cella del GridManager
        Grid->SetUnit(TargetPos.X, TargetPos.Y, SelectedUnitClass);

        // --- FINE LOGICA FLUIDA ---
        SelectedUnitClass->bHasMoved = true;

        // Pulizia grafica immediata della griglia
        Grid->ClearHighlights();

        // Notifica il GameMode (il GM aspetterà comunque la fine del movimento grazie a bIsMoving)
        ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GM) GM->CheckPlayerTurnEnd();
    }
}

void ATBGPlayerController::AttemptAttack(ATBGBaseUnit* Target)
{
    UE_LOG(LogTemp, Error, TEXT("ATTEMPT ATTACK CHIAMATO! Bersaglio: %s"), *GetNameSafe(Target));
    if (!SelectedUnitClass || SelectedUnitClass->bHasAttacked || !Grid) return;

    // Verifica distanza Manhattan
    int32 Dist =
        FMath::Abs(SelectedUnitClass->GetGridPosition().X - Target->GetGridPosition().X) +
        FMath::Abs(SelectedUnitClass->GetGridPosition().Y - Target->GetGridPosition().Y);
    if (Dist > SelectedUnitClass->GetAttackRange())
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGPlayerController::AttemptAttack Bersaglio fuori portata."));
        return;
    }
    Grid = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    // Spec: si può attaccare solo unità allo stesso livello o inferiore
    FIntPoint SelectedUnitClassPos = SelectedUnitClass->GetGridPosition();
    FIntPoint TargetPos = Target->GetGridPosition();
    int32 AttackerElevation = (int32)Grid->GetElevationLevel(SelectedUnitClassPos.X, SelectedUnitClassPos.Y);
    int32 TargetElevation = (int32)Grid->GetElevationLevel(TargetPos.X, TargetPos.Y);

    UE_LOG(LogTemp, Warning, TEXT("ATTACCO: %s (Z:%d) -> %s (Z:%d)"),*SelectedUnitClass->GetName(), AttackerElevation, *Target->GetName(), TargetElevation);

    if (AttackerElevation >= TargetElevation)
    {
        int32 DmgMin = SelectedUnitClass->GetDamageMin();
        int32 DmgMax = SelectedUnitClass->GetDamageMax();
        int32 Damage = FMath::RandRange(DmgMin, DmgMax);
        UE_LOG(LogTemp, Warning, TEXT("ATBGPlayerController::AttemptAttack Bersaglio Colpito."));
        Target->ApplyTakenDamage(Damage);
        if (ATBGPlayerController* PC = Cast<ATBGPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
        {
            PC->ShowUnitStatsFromUnit(Target);
            if (ATBGHUD* HUD = Cast<ATBGHUD>(GetHUD()))
                if (HUD)HUD->RefreshUnitsUI();
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("ATBGPlayerController::AttemptAttack Elevazione diversa, impossibile attaccare. "));
        return;
    }

    SelectedUnitClass->bHasAttacked = true;
    // Deseleziona dopo l'attacco e pulisce highlights
    SelectUnit(SelectedUnitClass);

    // Notifica GameMode per verificare se il turno deve finire
    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM) GM->CheckPlayerTurnEnd();
}

void ATBGPlayerController::RequestEndTurn()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ATBGGameMode* GM = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GM)
    {   UE_LOG(LogTemp, Error, TEXT("GM è NULL nel click!"));   }
    GM->OnTurnComplete();
}

bool ATBGPlayerController::GetInteractableUnderCursor(AActor*& OutActor) const
{
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        OutActor = Hit.GetActor();
        return OutActor != nullptr;
    }
    return false;
}


