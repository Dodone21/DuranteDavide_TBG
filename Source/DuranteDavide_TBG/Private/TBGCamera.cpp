// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGCamera.h"
#include "Camera/CameraComponent.h"

// Sets default values
ATBGCamera::ATBGCamera()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	// Attacchiamo la camera direttamente alla radice
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(RootComponent);

	// Pitch -90 guarda dritto verso terra
	MainCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	// Orientamento perfettamente verticale per vista top-down

	// Se vuoi una camera ortografica (stile RTS puro senza prospettiva)
	MainCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
;

}

// Called when the game starts or when spawned
void ATBGCamera::BeginPlay()
{
	Super::BeginPlay();
	// Un piccolo delay o chiamata post-generazione griglia è consigliato 
	// se la griglia non è ancora pronta in BeginPlay
	SetupCameraViewCentered();
	
}

// Called to bind functionality to input
void ATBGCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ATBGCamera::SetupCameraViewCentered()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Calcoliamo il centro: (25 celle * 100 unità) / 2 = 1250.0f
	float GridHalfSize = (GridCells * TileSize) / 2.0f;

	// Posizioniamo il Pawn al centro della griglia (X, Y) e all'altezza desiderata (Z)
	// Assumendo che la griglia parta da 0,0
	FVector NewLocation = FVector(GridHalfSize, GridHalfSize, ViewHeight);
	SetActorLocation(NewLocation);

	// Se usi la camera Ortografica, dobbiamo impostare quanto "terreno" vede
	if (MainCamera->ProjectionMode == ECameraProjectionMode::Orthographic)
	{
		// La larghezza ortografica copre l'intera griglia + il padding
		float OrthoWidth = (GridCells * TileSize) * ZoomPadding;
		MainCamera->SetOrthoWidth(OrthoWidth);
	}
}
