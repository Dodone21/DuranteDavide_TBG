// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"
#include "GridMapManager.h"
#include "TBGGameInstance.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;
	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	RootComponent = TileMesh;
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATile::SetGridPosition(int32 InX, int32 InY) 
{
	GridX = InX;
	GridY = InY;
    //Elevation = InElevation;

    UpdateVisuals();
}

FGridCell ATile::GetCell() const
{
    FGridCell Cell;
    if (!GetWorld()) return Cell;

    AGridMapManager* GM = Cast<AGridMapManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridMapManager::StaticClass()));
    const UTBGGameInstance* GI = Cast<UTBGGameInstance>(GetWorld()->GetGameInstance());

    if (!GI || !GM) return Cell;

    if (!GI->IsValidGridPosition(GridX, GridY))
        return Cell;

    int32 CurrentZ = GM->GetHeight(GridX, GridY);
    return GI->GetGridCell(GridX, GridY, CurrentZ);
}

void ATile::SetHighlight(bool bEnable) 
{
    bIsHighlighted = bEnable;
    // Ogni volta che lo stato cambia, dobbiamo ridisegnare la cella TUTTO IN BLUEPRINT
    UpdateVisuals();

}

FIntPoint ATile::GetGridPosition()
{
    return FIntPoint(GridX,GridY);
}

// void AST_Tile::OnInteract_Implementation()

// UpdateVisual e' implemenetata su BluePrint FUNZIONANTE 
// (da implementare il caso HIGHLIGHT
/*
void ATile::UpdateVisuals()
{
    if (!TileMesh) return;

    // Cerchiamo il materiale corrispondente all'elevazione nella mappa
    if (ElevationMaterials.Contains(Elevation))
    {
        UMaterialInterface* SelectedMaterial = ElevationMaterials[Elevation];
        if (SelectedMaterial)
        {
            TileMesh->SetMaterial(0, SelectedMaterial);
        }
    }
}


void AST_Tile::ToggleHighlight_Implementation(bool bIsVisible)
{
    TArray<UStaticMeshComponent*> Meshes;
    GetComponents<UStaticMeshComponent>(Meshes);

    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (Mesh)
        {
            // 1. Attiva/Disattiva il rendering nel buffer di profondità
            Mesh->SetRenderCustomDepth(bIsVisible);

            // 2. CORREZIONE: Se visibile metti 100, se NO metti 0
            int32 StencilValue = bIsVisible ? 100 : 0;
            Mesh->SetCustomDepthStencilValue(StencilValue);

            // 3. OPZIONALE MA CONSIGLIATO: Forza l'aggiornamento immediato
            Mesh->MarkRenderStateDirty();
        }
    }
}

*/