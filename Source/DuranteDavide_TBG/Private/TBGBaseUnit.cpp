// Fill out your copyright notice in the Description page of Project Settings.
    

#include "TBGBaseUnit.h"
#include "GridMapManager.h"
#include "TBGGameInstance.h"
#include "Tile.h"
#include "TBGTower.h"
#include "TBGGameMode.h"
#include "Kismet/GameplayStatics.h"


ATBGBaseUnit::ATBGBaseUnit()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
    RootComponent = UnitMesh;

    UnitGridPosition = FIntPoint(0, 0);
    TurnOwnerType = EUnitTurnOwner::None;
    bHasMoved = false;
    bHasAttacked = false;

}

void ATBGBaseUnit::BeginPlay()
{
	Super::BeginPlay();
    CurrentHP = MaxHP;

    UpdateTeamVisuals();
}

void ATBGBaseUnit::UpdateTeamVisuals()
{
    if (!UnitMesh) return;
    // Seleziona il materiale in base all'OwnerType
    UMaterialInterface* SelectedMat = (TurnOwnerType == EUnitTurnOwner::Human) ? HumanMaterial : AIMaterial;
    if (SelectedMat) UnitMesh->SetMaterial(0, SelectedMat);
}

void ATBGBaseUnit::SetGridPosition(int32 X, int32 Y)
{
    UnitGridPosition = FIntPoint(X, Y);
    //StartingUnitGridPosition = FIntPoint(X, Y);

    AGridMapManager* LocalGMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    if (LocalGMM)
    {
        if (!bIsMoving) 
        {
            FVector NewLocation = LocalGMM->GridToWorld(X, Y);
            // Offset fisso minimo per non far compenetrare la base della mesh con la tile
            NewLocation.Z += 100.0f;
            SetActorLocation(NewLocation);
        }

    }
}

void ATBGBaseUnit::ApplyTakenDamage(int32 Amount)
{
    if (Amount <= 0) return;

    CurrentHP -= Amount;
    UE_LOG(LogTemp, Log, TEXT("ATBGBaseUnit::ApplyTakenDamage - %s: %d PV rimanenti."), *GetName(), CurrentHP);

    if (CurrentHP <= 0)
    {
        CurrentHP = 0;
        OnDeath();
    }
}

void ATBGBaseUnit::VisibileActionMoving(const TArray<FIntPoint>& NewPath)
{
    GMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    UE_LOG(LogTemp, Warning, TEXT("MOVIMENTO VISIVO AVVIATO"));
    if (NewPath.Num() == 0 || !GMM) return;

    CurrentPathWorldPoints.Empty();
    for (FIntPoint Coord : NewPath) {
        // GridToWorld restituisce la posizione X, Y e Z corretta della cella
        FVector WorldLocation = GMM->GridToWorld(Coord.X, Coord.Y);

        // Offset per il pivot (a seconda della mesh)
        WorldLocation.Z += 100.0f;
        CurrentPathWorldPoints.Add(WorldLocation);
    }
    PathIndex = 0;
    bIsMoving = true;
}

void ATBGBaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving && CurrentPathWorldPoints.IsValidIndex(PathIndex))
    {
        FVector TargetLoc = CurrentPathWorldPoints[PathIndex]; // Il prossimo "checkpoint"
        // FORZIAMO L'ALTEZZA FISSA RISPETTO ALLA CELLA
        // Aggiungiamo l'offset alla Z della mattonella
        FVector CurrentLoc = GetActorLocation();

        // Direzione verso il prossimo punto
        FVector Direction = (TargetLoc - CurrentLoc).GetSafeNormal();
        float DistanceToTarget = FVector::Dist(CurrentLoc, TargetLoc);
        float FrameStep = MovementSpeed * DeltaTime;

        if (FrameStep >= DistanceToTarget)
        {
            // Abbiamo raggiunto il punto corrente, passiamo al prossimo
            SetActorLocation(TargetLoc);
            PathIndex++;

            if (PathIndex >= CurrentPathWorldPoints.Num())
            {
                bIsMoving = false;
                UE_LOG(LogTemp, Warning, TEXT("DEBUG: L'unità %s sta inviando il Broadcast di fine movimento!"), *GetName());
                // 2. NOTIFICHIAMO IL CONTROLLER
                // Se l'AI sta ascoltando, questo attiverà OnCurrentAIUnitActionFinished
                if (OnMovementFinished.IsBound()){   OnMovementFinished.Broadcast(); }
                UE_LOG(LogTemp, Log, TEXT("Movimento completato."));
            }
        }
        else
        {
            // Spostamento e rotazione fluida
            SetActorLocation(CurrentLoc + (Direction * FrameStep));

            // Rotazione fluida verso la direzione di marcia
            if (!Direction.IsNearlyZero()) {
                FRotator TargetRot = Direction.Rotation();
                TargetRot.Pitch = 0.0f;
                TargetRot.Roll = 0.0f;
                SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.0f));
            }
        }
    }

}

void ATBGBaseUnit::Move(int32 TargetX, int32 TargetY) 
{
    // Guard: questa unità ha già mosso questo turno
    if (bHasMoved)
    {
        UE_LOG(LogTemp, Warning,TEXT("ATBGBaseUnit::Move — %s ha già mosso questo turno."), *GetName());
        return;
    }

    // Recupera GridMapManager dalla scena
    GMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));

    if (!GMM)
    {
        UE_LOG(LogTemp, Error, TEXT("ATBGBaseUnit::Move — GridMapManager non trovato."));
        return;
    }

    // Guard: destinazione valida e calpestabile
    if (!GMM->IsValidCell(TargetX, TargetY) || !GMM->IsWalkable(TargetX, TargetY))
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Move — Destinazione (%d,%d) non valida."), TargetX, TargetY);
        return;
    }

    // Chiedi a Pathfinder il percorso e il costo totale
    // Pathfinder::FindPath restituisce il costo totale tenendo
    // conto di piano=1, salita=2, discesa=1.
    // Se non esiste percorso, ritorna -1.
    int32 PathCost = 0;
    TArray<FIntPoint> GridPath = GMM->GetPath(UnitGridPosition, FIntPoint(TargetX, TargetY), PathCost);
    if (PathCost < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Move — Nessun percorso verso (%d,%d)."), TargetX, TargetY);
        return;
    }

    if (PathCost > MovementRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Move — Percorso costo %d supera MovementRange %d."), PathCost, MovementRange);
        return;
    }

    // Aggiorna griglia logica
    GMM->ClearUnit(UnitGridPosition.X, UnitGridPosition.Y);
    UnitGridPosition = FIntPoint(TargetX, TargetY);
    GMM->SetUnit(TargetX, TargetY, this);

    // Prepara i punti per il Tick (Animazione)
    CurrentPathWorldPoints.Empty();
    for (FIntPoint Point : GridPath)
    {
        CurrentPathWorldPoints.Add(GMM->GridToWorld(Point.X, Point.Y));
    }

    bIsMoving = true;
    bHasMoved = true;
}

int32 ATBGBaseUnit::Attack(ATBGBaseUnit* TargetUnit)
{
    // Guard: questa unità ha già attaccato questo turno
    if (bHasAttacked)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Attack — %s ha già attaccato questo turno."),*GetName());
        return 0;
    }

    // Guard: bersaglio valido
    if (!TargetUnit)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Attack — TargetUnit è nullptr."));
        return 0;
    }

    // Guard: non si può attaccare un'unità alleata
    if (TargetUnit->TurnOwnerType == TurnOwnerType)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Attack — Impossibile attaccare un'unità alleata."));
        return 0;
    }

    // Recupera GridMapManager per i check di elevazione e distanza
    AGridMapManager* GridManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("ATBGBaseUnit::Attack — GridMapManager non trovato."));
        return 0;
    }

    int32 AttackerX = UnitGridPosition.X;
    int32 AttackerY = UnitGridPosition.Y;
    int32 TargetX = TargetUnit->UnitGridPosition.X;
    int32 TargetY = TargetUnit->UnitGridPosition.Y;

    int32 DiffX = FMath::Abs(TargetX - AttackerX);
    int32 DiffY = FMath::Abs(TargetY - AttackerY);
    // dist Chebyshev
    int32 Distance = FMath::Max(DiffX, DiffY);


    if (Distance > AttackRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::Attack — Bersaglio fuori range (dist=%d, range=%d)."), Distance, AttackRange);
        return 0;
    }

    // Attacco a distanza: verifica line-of-sight sopra l'acqua
    // (lo Sniper può oltrepassarla, il Brawler no — ma il Brawler
    // ha range 1 quindi in pratica non raggiunge mai l'acqua)
  
    // Estrai il danno casuale nel range [DamageMin, DamageMax]
    int32 Damage = FMath::RandRange(DamageMin, DamageMax);

    UE_LOG(LogTemp, Log, TEXT("ATBGBaseUnit::Attack — %s attacca %s per %d danni."), *GetName(), *TargetUnit->GetName(), Damage);

    TargetUnit->ApplyTakenDamage(Damage);
    bHasAttacked = true; 
    return Damage;
}

void ATBGBaseUnit::OnDeath()
{
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    UE_LOG(LogTemp, Log, TEXT("ATBGBaseUnit::OnDeath — è stata eliminata."));
    bIsDead = true;

    // LIBERA IL DEFENDER DELLE TORRI SE L'UNITÀ MORTA È AI
    if (GetTurnOwnerType() == EUnitTurnOwner::AI)
    {
        TArray<AActor*> FoundTowers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATBGTower::StaticClass(), FoundTowers);

        for (AActor* Actor : FoundTowers)
        {
            ATBGTower* Tower = Cast<ATBGTower>(Actor);

            if (!IsValid(Tower))    continue;

            if (Tower->AssignedDefender == this)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("AI Defender rimosso dalla torre %s"),
                    *Tower->GetName());

                Tower->AssignedDefender = nullptr;
            }
        }
    }

    // Notifica GameMode — sarà lei a gestire il respawn
    ATBGGameMode* GameMode = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GMM = GameMode->GetGridMapManager();
        if (GMM) {
            GMM->SetUnit(UnitGridPosition.X, UnitGridPosition.Y, nullptr);
            GMM->ClearUnit(UnitGridPosition.X, UnitGridPosition.Y);
            //SetGridPosition(StartingUnitGridPosition.X, StartingUnitGridPosition.Y);
            UE_LOG(LogTemp, Warning, TEXT("Cella [%d, %d] liberata dopo la morte di %s"), UnitGridPosition.X, UnitGridPosition.Y, *GetName())
        }
        GameMode->OnUnitDied(this);
    }
}

void ATBGBaseUnit::RespawnReset()
{
    UE_LOG(LogTemp, Warning, TEXT("ATBGBaseUnit::RespawnReset"));
    // Ripristina i punti vita al valore massimo definito per la classe 
    CurrentHP = MaxHP;
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    bIsDead = false;
    // Riporta l'unità alla posizione di partenza salvata all'inizio della partita

    ATBGGameMode* GameMode = Cast<ATBGGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        SetGridPosition(StartingUnitGridPosition.X, StartingUnitGridPosition.Y);
        AGridMapManager* LocalGMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
        if(LocalGMM) LocalGMM->SetUnit(StartingUnitGridPosition.X, StartingUnitGridPosition.Y, this);
    }
    // Se l'unità era stata nascosta visivamente, rendila di nuovo visibile
    ResetTurnState();
}

void ATBGBaseUnit::ConfirmStartingPosition()
{
    StartingUnitGridPosition = UnitGridPosition;
    UE_LOG(LogTemp, Log, TEXT("Unità %s: Posizione iniziale fissata a %s"), *GetName(), *StartingUnitGridPosition.ToString());
}

void ATBGBaseUnit::ResetTurnState()
{
    bHasMoved = false;
    bHasAttacked = false;
    bIsMoving = false;

    UE_LOG(LogTemp, Verbose, TEXT("ATBGBaseUnit::ResetTurnState — pronto per il turno."));
}

void ATBGBaseUnit::ToggleHighlightFunct(bool SToggleHighlight)
{
    ToggleHighlight = SToggleHighlight;
    int32 X = UnitGridPosition.X;
    int32 Y = UnitGridPosition.Y;

    AGridMapManager* GridManager = Cast<AGridMapManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));; 

    if (GridManager)
    {
        ATile* CurrentTile = GridManager->GetTile(X, Y);
        if (CurrentTile)
        {
            CurrentTile->SetHighlight(SToggleHighlight);
        }
    }
}

FGridCell ATBGBaseUnit::GetCurrentCell() const
{
    // 1. Get X and Y from your FIntPoint
    int32 GridX = UnitGridPosition.X;
    int32 GridY = UnitGridPosition.Y;
    int32 GridZ = 1;

    if (const UWorld* World = GetWorld())
    {
        if (const UTBGGameInstance* GI = Cast<UTBGGameInstance>(World->GetGameInstance()))
        {
            // Passiamo le coordinate correnti inclusa la Z dinamica
            return GI->GetGridCell(GridX, GridY, GridZ);
        }
    }
    return FGridCell(GridX, GridY, GridZ);
}
