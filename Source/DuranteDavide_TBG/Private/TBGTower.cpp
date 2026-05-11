// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGTower.h"
#include "GridMapManager.h"
#include "TBGGameInstance.h"
#include "TBGBaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"


ATBGTower::ATBGTower()
{
	PrimaryActorTick.bCanEverTick = false;

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TowerMesh"));
	RootComponent = TowerMesh;
}

void ATBGTower::BeginPlay()
{
	Super::BeginPlay();

	if (GridX >= 0 && GridY >= 0)
	{	SetGridPosition(GridX, GridY);	}
	UpdateVisuals();
	
}

void ATBGTower::SetGridPosition(int32 InX, int32 InY)
{
	UWorld* World = GetWorld();
	if (!World) return;

	GridX = InX;
	GridY = InY;

	AGridMapManager* GMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(World, AGridMapManager::StaticClass()));
	if (GMM)
	{
		FVector TargetLocation = GMM->GridToWorld(InX, InY);
		SetActorLocation(TargetLocation);
		GMM->SetTower(InX, InY, this);
	}
}

void ATBGTower::SetControlledBy(EUnitTurnOwner NewOwner)
{
	TowerState = (NewOwner == EUnitTurnOwner::None)
		? ETowerState::Neutral
		: ETowerState::Controlled;

	UpdateVisuals();
	UE_LOG(LogTemp, Warning, TEXT("Tower [%d,%d] → Owner=%d State=%d"), GridX, GridY, (int32)NewOwner, (int32)TowerState);
}

// DEVI IMPLEMENTARE ANCORA TUTTE LE MESH
void ATBGTower::UpdateVisuals()
{
	if (!TowerMesh) return;

	UMaterialInterface* SelectedMat = NeutralMaterial;

	// Se la torre è contesa
	if (TowerState == ETowerState::Contested)
	{
		SelectedMat = ContestedMaterial;
	}
	else {
		// In base al proprietario
		switch (TurnControlledBy)
		{
		case EUnitTurnOwner::Human: SelectedMat = HumanMaterial; break;
		case EUnitTurnOwner::AI:    SelectedMat = AIMaterial; break;
		default:                SelectedMat = NeutralMaterial; break;
		}
	}

	if (SelectedMat) {
		TowerMesh->SetMaterial(0, SelectedMat);
	}
}

void ATBGTower::EvaluateEEUpdateTowerControlState(const TArray<class ATBGBaseUnit*>& NearbyUnits)
{
	int32 HumanCount = 0;
	int32 AICount = 0;

	if (NearbyUnits.Num() == 0) 
	{ 
		TowerState = ETowerState::Neutral;  
		TurnControlledBy = EUnitTurnOwner::None;
		return; 
	} // Se non c'è nessuno, mantiene lo stato attuale

	for (ATBGBaseUnit* Units : NearbyUnits)	
	{
		if (Units->GetTurnOwnerType() == EUnitTurnOwner::Human) HumanCount++;
		else if (Units->GetTurnOwnerType() == EUnitTurnOwner::AI) AICount++;
	}

	if (HumanCount > 0 && AICount > 0)
	{	TowerState = ETowerState::Contested; TurnControlledBy = EUnitTurnOwner::None;	}
	else if (HumanCount > 0)
	{	TowerState = ETowerState::Controlled; TurnControlledBy = EUnitTurnOwner::Human; }
	else if (AICount > 0)
	{	TowerState = ETowerState::Controlled; TurnControlledBy = EUnitTurnOwner::AI;	}

	UpdateVisuals();
}

TArray<ATBGBaseUnit*> ATBGTower::GetUnitsInCaptureZone() const
{
	UE_LOG(LogTemp, Log, TEXT("ATBGTower::GetUnitsInCaptureZone chiamato"));

	TArray<ATBGBaseUnit*> FoundUnits;
	AGridMapManager* GridManager = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
	if (!GridManager) return FoundUnits;

	// Definiamo il raggio di cattura
	const int32 CaptureRadius = 2;

	// Scansione del quadrato 5x5 centrato sulla torre
	for (int32 dx = -CaptureRadius; dx <= CaptureRadius; dx++)
	{
		for (int32 dy = -CaptureRadius; dy <= CaptureRadius; dy++)
		{
			// Saltiamo la cella (0,0) perché è la torre stessa (ostacolo)
			if (dx == 0 && dy == 0) continue;

			FIntPoint CheckPos(GetGridX() + dx, GetGridY() + dy);

			// Verifichiamo che la cella sia dentro i confini della mappa (0-24)
			if (GridManager->IsValidCell(CheckPos.X, CheckPos.Y))
			{
				// Chiediamo al manager se c'è un'unità in questa cella
				ATBGBaseUnit* UnitOnTile = GridManager->GetUnit(CheckPos.X, CheckPos.Y);
				if (UnitOnTile)
				{
					FoundUnits.Add(UnitOnTile);
				}
			}
		}
	}

	return FoundUnits;
}

void ATBGTower::SolvePlacement(UObject* WorldContextObject, AGridMapManager* GridManager, TSubclassOf<ATBGTower> TowerClass)
{
	if (!GridManager || !TowerClass) return;

	UWorld* World = GEngine->GetWorldFromContextObject(
		WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	// Posizioni ideali da specifiche:
	//   Centro (12,12)  |  Lato Sinistro (5,12)  |  Lato Destro (19,12)
	const TArray<FIntPoint> IdealPoints = {
		FIntPoint(12, 12),
		FIntPoint(5, 12),
		FIntPoint(19, 12)
	};

	UE_LOG(LogTemp, Warning, TEXT("Inizio SolvePlacement..."));
	for (const FIntPoint& Ideal : IdealPoints)
	{
		if (!GridManager->IsValidCell(Ideal.X, Ideal.Y)) continue;

		// Se la cella ideale non è walkable, cerca la più vicina
		const FIntPoint FinalPos = FindNearestValidTowerSpot(GridManager, Ideal);
		UE_LOG(LogTemp, Warning, TEXT("Punto finale trovato: %s"), *FinalPos.ToString());

		// Posizione 3D: usa GridToWorld del GridManager (gestisce già Height)
		FVector SpawnLocation = GridManager->GridToWorld(FinalPos.X, FinalPos.Y);
		SpawnLocation.Z += 10.f; // offset anti-compenetrazione

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATBGTower* NewTower = World->SpawnActor<ATBGTower>(TowerClass, SpawnLocation, FRotator::ZeroRotator, Params);

		if (NewTower)
		{
			// SetGridPosition si occupa già di chiamare GM->SetTower internamente
			NewTower->SetGridPosition(FinalPos.X, FinalPos.Y);

			UE_LOG(LogTemp, Log, TEXT("Tower spawned at [%d,%d]"), FinalPos.X, FinalPos.Y);
		}
	}
}

FIntPoint ATBGTower::FindNearestValidTowerSpot(AGridMapManager* GridManager, FIntPoint Ideal)
{
	// Cella ideale già valida? Usala subito.
	if (GridManager->IsWalkable(Ideal.X, Ideal.Y))
		return Ideal;

	// Ricerca a spirale (raggio crescente) per trovare la terraferma più vicina
	constexpr int32 MaxRadius = 4;

	for (int32 Radius = 1; Radius <= MaxRadius; ++Radius)
	{
		for (int32 dx = -Radius; dx <= Radius; ++dx)
		{
			for (int32 dy = -Radius; dy <= Radius; ++dy)
			{
				// Considera solo il perimetro del quadrato corrente
				if (FMath::Abs(dx) != Radius && FMath::Abs(dy) != Radius) continue;

				const int32 X = Ideal.X + dx;
				const int32 Y = Ideal.Y + dy;

				if (GridManager->IsValidCell(X, Y) && GridManager->IsWalkable(X, Y))
					return FIntPoint(X, Y);
			}
		}
	}

	// Fallback estremo: ritorna il punto ideale originale
	UE_LOG(LogTemp, Warning, TEXT("FindNearestValidTowerSpot: nessuna cella walkable trovata vicino a [%d,%d]"), Ideal.X, Ideal.Y);
	return Ideal;
}

FGridCell ATBGTower::GetCell() const
{
	FGridCell Cell;
	Cell.X = GridX;
	Cell.Y = GridY;

	int32 TowerZ = 1;
	if (const UWorld* World = GetWorld())
	{
		AGridMapManager* GMM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(World, AGridMapManager::StaticClass()));

		if (GMM)
			TowerZ = GMM->GetHeight(GridX, GridY);

		if (UTBGGameInstance* GI = Cast<UTBGGameInstance>(World->GetGameInstance()))
			Cell = GI->GetGridCell(GridX, GridY, TowerZ);
	}

	Cell.Z = TowerZ;
	Cell.bIsTowerCell = true;
	Cell.Tower = const_cast<ATBGTower*>(this);

	return Cell;
}

FIntPoint ATBGTower::GetGridPosition() const
{
	return FIntPoint(GridX, GridY);
}

bool ATBGTower::IsUnitInsideCaptureRange(ATBGBaseUnit* Unit) const
{
	if (!IsValid(Unit))	return false;

	const FIntPoint TowerPos = GetGridPosition();
	const FIntPoint UnitPos = Unit->GetGridPosition();

	const int32 DX = FMath::Abs(UnitPos.X - TowerPos.X);
	const int32 DY = FMath::Abs(UnitPos.Y - TowerPos.Y);

	// Range 2 celle
	const bool bInside = DX <= 2 && DY <= 2 && !(DX == 0 && DY == 0);

	UE_LOG(LogTemp, Warning,
		TEXT("TowerRangeCheck: Unit(%d,%d) Tower(%d,%d) -> %s"),
		UnitPos.X,
		UnitPos.Y,
		TowerPos.X,
		TowerPos.Y,
		bInside ? TEXT("TRUE") : TEXT("FALSE"));

	return bInside;
}
