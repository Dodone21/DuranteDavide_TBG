// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "TBGCamera.generated.h"

class UCameraComponent;

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATBGCamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Centra la camera e imposta l'OrthoWidth in base alle dimensioni attuali
    // della griglia recuperate dalla GameInstance.
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetupCameraViewCentered();

protected:
    // La camera è ora l'unico componente necessario
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical Camera")
    UCameraComponent* MainCamera;

    UPROPERTY(EditAnywhere, Category = "Grid Config")
    float ViewHeight = 2000.0f;

    // Parametri della griglia
    UPROPERTY(EditAnywhere, Category = "Grid Config")
    int32 GridCells = 25;

    UPROPERTY(EditAnywhere, Category = "Grid Config")
    float TileSize = 100.0f; // Dimensione di una singola cella (es. 1 metro)

    UPROPERTY(EditAnywhere, Category = "Grid Config")
    float ZoomPadding = 1.1f; // Moltiplicatore per non avere la griglia a filo bordo

};