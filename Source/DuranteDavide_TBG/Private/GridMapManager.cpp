// Fill out your copyright notice in the Description page of Project Settings.

#include "GridMapManager.h"
#include "TBGGameInstance.h"
#include "Tile.h"
#include "TBGTower.h"
#include "TBGBaseUnit.h"
#include "TBGCamera.h"
#include "TBGBaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"


AGridMapManager::AGridMapManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridMapManager::BeginPlay()
{
	Super::BeginPlay();

    // Recupera i settaggi dalla GameInstance
    if (UTBGGameInstance* GI = Cast<UTBGGameInstance>(GetGameInstance()))
    {
        GridSettings = GI->GetMapSettings();
        UE_LOG(LogTemp, Warning, TEXT("GridManager: Generazione mappa con Seed: %d"), GridSettings.RandomSeed);
    }

}

void AGridMapManager::GenerateGrid()
{
    // Recupero parametri dal GameInstance
    UTBGGameInstance* GI = Cast<UTBGGameInstance>(GetWorld()->GetGameInstance());
    if (!GI) return;

    GridSettings = GI->GetMapSettings();

    // Pulizia totale della griglia esistente
    for (ATile* T : Tiles) { if (T) T->Destroy(); }
    Tiles.Empty();
    // Pulizia riferimenti alle torri prima di svuotare la griglia logica
    Grid.Empty();
    Grid.SetNum(GridSettings.MapWidth * GridSettings.MapHeight);

    // Generazione Mappa con Perlin Noise
    FRandomStream Stream(GridSettings.RandomSeed);
    float OffsetX = Stream.FRandRange(-10000.f, 10000.f);
    float OffsetY = Stream.FRandRange(-10000.f, 10000.f);

    for (int32 Y = 0; Y < GridSettings.MapHeight; Y++)
    {
        for (int32 X = 0; X < GridSettings.MapWidth; X++)
        {
            float Noise = FMath::PerlinNoise2D(FVector2D(X + OffsetX, Y + OffsetY) * GridSettings.PerlinScale);
            // Usiamo un range leggermente più aggressivo
            Noise = (Noise * 1.2f); // Amplifica le oscillazioni
            Noise = FMath::Clamp((Noise + 1.0f) * 0.5f, 0.0f, 1.0f); // Riporta in [0,1] e taglia gli eccessi

            // Queste soglie sono basate sul feedback visivo (più acqua, più picchi, meno giallo)
            int32 H;
            if (Noise < 0.25f)      H = 0; // Acqua (Blue) - Aumentato per avere più acqua
            else if (Noise < 0.45f) H = 1; // Terra (Verde)
            else if (Noise < 0.55f) H = 2; // Montagna (Giallo) - Ristretto perché è il range più comune
            else if (Noise < 0.75f) H = 3; // Montagna (Orange)
            else H = 4; // Montagna (Red) - Aumentato per avere più picchi

            int32 Idx = GetIDCell(X, Y);
            Grid[Idx] = FGridCell(X, Y, H);
            Grid[Idx].ElevationLevel = static_cast<EElevationLevel>(H);
            Grid[Idx].bIsWater = (H == 0);
        }
    }

    // Forza le zone delle torri e i cammini tra di esse a essere terra (H=1)
    TArray<FIntPoint> KeyPoints = { FIntPoint(5,12), FIntPoint(12,12), FIntPoint(19,12) };
    for (FIntPoint P : KeyPoints)
    {
        if (IsValidCell(P.X, P.Y))
        {
            int32 Idx = GetIDCell(P.X, P.Y);
            // COntrolliamo SOLO sse e' acqua
            if (Grid[Idx].bIsWater)
            {
                Grid[Idx].ElevationLevel = EElevationLevel::Flat;
                Grid[Idx].bIsWater = false;
                UE_LOG(LogTemp, Warning, TEXT("Cella %s era acqua. Metto Flat e Ricerca di una posizione migliore."), *P.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Cella %s è già emersa, lascio invariato."), *P.ToString());
            }
        }
    }

    // Corregge la Grid logica affondando le isole prima che vengano create visivamente
    EnsureReachabilityOfMap(12, 12);

    // Spawn visivo della Tile
    if (TileClass)
    {
        for (int32 i = 0; i < Grid.Num(); i++) {
            FGridCell& Cell = Grid[i];

            FVector SpawnLoc = GridToWorld(Cell.X, Cell.Y);
            FRotator Rotation = FRotator::ZeroRotator;
            FActorSpawnParameters Params;

            ATile* NewTile = GetWorld()->SpawnActor<ATile>(TileClass, SpawnLoc, Rotation, Params);
            if (NewTile)
            {
                NewTile->SetGridPosition(Cell.X, Cell.Y);
                NewTile->Elevation = Cell.ElevationLevel;
                NewTile->UpdateVisuals();
                Tiles.Add(NewTile);
            }
        }
    }

    // Chiamata al solveplacement di Tower
    if (TowerClass)
    {
        ATBGTower::SolvePlacement(GetWorld(), this, TowerClass);
        UE_LOG(LogTemp, Log, TEXT("GridManager: Solver delle torri eseguito."));
    }

    // Setup Camera
    if (ATBGCamera* Camera = Cast<ATBGCamera>(UGameplayStatics::GetActorOfClass(GetWorld(), ATBGCamera::StaticClass())))
    {
        Camera->SetupCameraViewCentered();
    }
}

bool AGridMapManager::IsValidCell(int32 InX, int32 InY) const
{
	return InX >= 0 && InX < GridSettings.MapWidth && InY >= 0 && InY < GridSettings.MapHeight;
}

bool AGridMapManager::IsWalkable(int32 InX, int32 InY) const
{
	if (!IsValidCell(InX, InY)) return false;
	const FGridCell& C = Grid[GetIDCell(InX, InY)];
	return (C.Z != (int32)EElevationLevel::Water && C.Tower == nullptr && C.OccupyingUnit == nullptr);
}

bool AGridMapManager::IsTowerAt(int32 InX, int32 InY) const
{
    return IsValidCell(InX, InY) && Grid[GetIDCell(InX, InY)].Tower != nullptr;
}

EElevationLevel AGridMapManager::GetElevationLevel(int32 InX, int32 InY) const
{
    // Se la cella non è valida, restituiamo un valore di default (es. Water)
    if (!IsValidCell(InX, InY))
    {
        return EElevationLevel::Water;
    }

    // Accediamo alla cella e restituiamo direttamente la variabile Enum
    return Grid[GetIDCell(InX, InY)].ElevationLevel;
}

int32 AGridMapManager::GetIDCell(int32 InX, int32 InY) const
{
	return InY * GridSettings.MapWidth + InX;
}

// MI DARA' SEMPRE 0
int32 AGridMapManager::GetHeight(int32 InX, int32 InY) const
{
    return IsValidCell(InX, InY) ? Grid[GetIDCell(InX, InY)].Z : 0;
}

FGridCell AGridMapManager::GetCell(int32 X, int32 Y) const
{
	return IsValidCell(X, Y) ? Grid[GetIDCell(X, Y)] : FGridCell();
}

void AGridMapManager::HighlightRange(ATBGBaseUnit* Unit)
{
    if (!Unit) return;

    // 1. Pulizia totale degli highlight precedenti
    ClearHighlights();

    FIntPoint StartPos = Unit->UnitGridPosition;
    int32 MoveRange = Unit->GetMovementRange();

    // Ciclo su tutta la griglia
    for (int32 x = 0; x < GridSettings.MapWidth; x++)
    {
        for (int32 y = 0; y < GridSettings.MapHeight; y++)
        {
            FIntPoint TargetPos(x, y);

            // Saltiamo la cella dove si trova l'unità stessa
            if (TargetPos == StartPos) continue;

            ATile* TileActor = GetTile(x, y);
            if (!TileActor) continue;

            if (!IsWalkable(x, y)) continue;

            int32 TotalPathCost = 0;
            TArray<FIntPoint> Path = GetPath(StartPos, TargetPos, TotalPathCost);

            // Se esiste un percorso (Path non vuoto) e il costo rientra nel range dell'unità
            if (Path.Num() > 0 && TotalPathCost <= MoveRange)
            {
                //ATile* TileActor = GetTile(x, y); e' inutile poiche' e' gia' indicata prima
                if (TileActor)
                {
                    // Specifica: evidenzia la cella con un colore diverso [cite: 255, 289]
                    // Assumendo che tu abbia un'interfaccia o una funzione per l'highlight
                    //ToggleTileHighlight(TileActor, true);// DA IMPLEMENTARE
                    TileActor->SetHighlight(true);
                }
            }
        }
    }
}

void AGridMapManager::ClearHighlights()
{
    for (int32 x = 0; x < GridSettings.MapWidth; x++)
    {
        for (int32 y = 0; y < GridSettings.MapHeight; y++)
        {
            ATile* TileActor = GetTile(x, y);
            if (TileActor)
            {
               TileActor->SetHighlight(false);
            }
        }
    }
}

bool AGridMapManager::IsInDeploymentZone(int32 Y, bool bIsHumanUnit) const
{
    if (bIsHumanUnit)
    {
        return (Y >= 0 && Y <= 2); 
    }
    else
    {
        return (Y >= 22 && Y <= 24); 
    }
}

TArray<FIntPoint> AGridMapManager::GetPath(FIntPoint Start, FIntPoint End, int32& OutTotalCost) const
{
    OutTotalCost = -1; // Valore di default se fallisce
    TArray<FIntPoint> FullPath;

    if (Start == End)
    {
        OutTotalCost = 0;
        return FullPath;
    }

    TMap<FIntPoint, int32> MinCostMap;
    TMap<FIntPoint, FIntPoint> ParentMap;
    TArray<FIntPoint> PriorityQueue;

    MinCostMap.Add(Start, 0);
    PriorityQueue.Add(Start);

    while (PriorityQueue.Num() > 0)
    {
        PriorityQueue.Sort([&MinCostMap](const FIntPoint& A, const FIntPoint& B) {
            return MinCostMap[A] < MinCostMap[B];
            });

        FIntPoint Current = PriorityQueue[0];
        PriorityQueue.RemoveAt(0);

        // Se abbiamo raggiunto la destinazione, SALVIAMO IL COSTO e fermiamo la ricerca
        if (Current == End)
        {
            OutTotalCost = MinCostMap[Current];
            break;
        }

        // Esploriamo le 4 direzioni (Niente obliquo)
        TArray<FIntPoint> Neighbors = {
            FIntPoint(Current.X + 1, Current.Y),
            FIntPoint(Current.X - 1, Current.Y),
            FIntPoint(Current.X, Current.Y + 1),
            FIntPoint(Current.X, Current.Y - 1)
        };

        for (FIntPoint Neighbor : Neighbors)
        {
            // Usa il tuo IsWalkable come facevi in GetMovementCost
            if (IsWalkable(Neighbor.X, Neighbor.Y))
            {
                int32 NewCost = MinCostMap[Current] + CalculateStepCost(Current, Neighbor);

                if (!MinCostMap.Contains(Neighbor) || NewCost < MinCostMap[Neighbor])
                {
                    MinCostMap.Add(Neighbor, NewCost);
                    ParentMap.Add(Neighbor, Current);
                    PriorityQueue.AddUnique(Neighbor);
                }
            }
        }
    }

    // Se la cella End non è nella ParentMap, non c'è percorso (OutTotalCost resta -1)
    if (!ParentMap.Contains(End)) return FullPath;

    // RICOSTRUZIONE DEL PERCORSO
    FIntPoint Temp = End;
    while (Temp != Start)
    {
        FullPath.Insert(Temp, 0);
        Temp = ParentMap[Temp];
    }
    FullPath.Insert(Start, 0);

    return FullPath;
}

int32 AGridMapManager::CalculateStepCost(FIntPoint From, FIntPoint To) const
{
    FGridCell FromTile = GetCell(From.X, From.Y);
    FGridCell ToTile = GetCell(To.X, To.Y);
    if (!IsWalkable(To.X, To.Y)) return 333;
    if (static_cast<int32>(ToTile.ElevationLevel) > static_cast<int32>(FromTile.ElevationLevel))
    {
        return 2; // Salita
    }
    return 1; // Piano o Discesa
}

FVector AGridMapManager::GridToWorld(int32 X, int32 Y) const
{
    float WorldY = X * GridSettings.CellSize;
    float WorldX = Y * GridSettings.CellSize;
    // Imposto altezza mondo a Z = 0, cosi' non avro nessuna cella alzata
    float WorldZ = 0.f;

    return FVector(WorldX, WorldY, WorldZ);

}

FString AGridMapManager::GetAlphaNumericCoords(int32 X, int32 Y) const
{ 
    // Trasformiamo X in una lettera (0 = A, 1 = B, ecc.)
    // 'A' ha valore ASCII 65. Sommando X otteniamo la lettera corrispondente.
    TCHAR ColumnLetter = static_cast<TCHAR>('A' + X);

    // Creiamo la stringa formattata.
    // Usiamo Y come numero della riga (es. X=5, Y=12 -> "F12")
    return FString::Printf(TEXT("%c%d"), ColumnLetter, Y);
}

void AGridMapManager::SetUnit(int32 X, int32 Y, ATBGBaseUnit* Unit)
{
    if (IsValidCell(X, Y) && IsWalkable(X,Y))
    {
        Grid[GetIDCell(X, Y)].OccupyingUnit = Unit;
        Grid[GetIDCell(X, Y)].bIsOccupiedByUnit = true;

        UE_LOG(LogTemp, Log, TEXT("Unità settata in cella (%d, %d)"), X, Y);
    }
}

void AGridMapManager::ClearUnit(int32 X, int32 Y)
{
    if (IsValidCell(X, Y))
    {
        Grid[GetIDCell(X, Y)].OccupyingUnit = nullptr;
        Grid[GetIDCell(X, Y)].bIsOccupiedByUnit = false;

        UE_LOG(LogTemp, Log, TEXT("Cella (%d, %d) liberata"), X, Y);
    }
}

ATBGBaseUnit* AGridMapManager::GetUnit(int32 X, int32 Y) const
{
    return IsValidCell(X, Y) ? Grid[GetIDCell(X, Y)].OccupyingUnit : nullptr;
}

void AGridMapManager::SetTower(int32 X, int32 Y, ATBGTower* Tower)
{
    if (IsValidCell(X, Y)) Grid[GetIDCell(X, Y)].Tower = Tower;
    Grid[GetIDCell(X, Y)].bIsTowerCell = true;
}

void AGridMapManager::EnsureReachabilityOfMap(int32 StartX, int32 StartY)
{
    UE_LOG(LogTemp, Log, TEXT("EnsureReachabilityOfMap "));
    // 1. Array per tracciare quali celle abbiamo visitato
    TArray<bool> Reachable;
    Reachable.SetNumZeroed(GridSettings.MapWidth * GridSettings.MapHeight);

    // 2. Coda per l'algoritmo (Breadth-First Search)
    TArray<int32> Queue;

    int32 StartIdx = GetIDCell(StartX, StartY);
    if (Grid.IsValidIndex(StartIdx) && Grid[StartIdx].ElevationLevel != EElevationLevel::Water)
    {
        Queue.Add(StartIdx);
        Reachable[StartIdx] = true;
    }

    // 3. Flood Fill: esploriamo la terra calpestabile
    int32 Head = 0;
    while (Head < Queue.Num())
    {
        int32 CurrentIdx = Queue[Head++];
        int32 CX = Grid[CurrentIdx].X;
        int32 CY = Grid[CurrentIdx].Y;

        // Controlliamo i 4 vicini (Nord, Sud, Est, Ovest)
        int32 DX[] = { 1, -1, 0, 0 };
        int32 DY[] = { 0, 0, 1, -1 };

        for (int i = 0; i < 4; i++)
        {
            int32 NX = CX + DX[i];
            int32 NY = CY + DY[i];
            int32 NextIdx = GetIDCell(NX, NY);

            if (Grid.IsValidIndex(NextIdx) && !Reachable[NextIdx])
            {
                // Se la cella NON è acqua (Elevation > 0), è raggiungibile
                if (Grid[NextIdx].ElevationLevel != EElevationLevel::Water)
                {
                    Reachable[NextIdx] = true;
                    Queue.Add(NextIdx);
                }
            }
        }
    } 

    // 4. Correzione: Ogni cella di terra NON raggiunta viene trasformata in acqua
    for (int32 i = 0; i < Grid.Num(); i++)
    {
        if (Grid[i].ElevationLevel != EElevationLevel::Water && !Reachable[i])
        {
            Grid[i].ElevationLevel = EElevationLevel::Water; // Affonda l'isola irraggiungibile
            Grid[i].bIsWater = true;
        }
    }
}

ATile* AGridMapManager::GetTile(int32 X, int32 Y) const
{
    if (!IsValidCell(X, Y)) return nullptr;
    int32 Idx = GetIDCell(X, Y);
    if (Tiles.IsValidIndex(Idx)){   return Tiles[Idx];  }
    return nullptr;
}

