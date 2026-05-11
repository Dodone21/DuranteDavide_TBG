// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TBGGameInstance.h"
#include "GridMapManager.h"
#include "TBGBaseUnit.generated.h"

//class AGridMapManager;
class ATBGTower;
class ATBGPlayerController;
class ATBGAIController;
class ATBGGameStateBase;
class ATBGHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementFinished);

UCLASS(Abstract)
class DURANTEDAVIDE_TBG_API ATBGBaseUnit : public APawn
{
	GENERATED_BODY()

public:
	ATBGBaseUnit();

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// --- DATI STATISTICI DELLE UNITA' ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Stats")
		int32 MaxHP = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Stats")
		int32 CurrentHP = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Stats")
		int32 MovementRange = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Stats")
		int32 AttackRange = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Stats")
		int32 DamageMin = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit|Stats")
		int32 DamageMax = 0;

	//Posizione(X, Y) dell'unità sulla griglia
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Grid")
	FIntPoint UnitGridPosition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Grid")
	FIntPoint StartingUnitGridPosition;

	// ----- GETTERS DEI DATI DELLE UNITA' ------
	UFUNCTION(BlueprintCallable)
	int32 GetMaxHP() const { return MaxHP; }
	UFUNCTION(BlueprintCallable)
	int32 GetCurrentHP() const { return CurrentHP; }
	UFUNCTION(BlueprintCallable)
	int32 GetMovementRange() const { return MovementRange; }
	UFUNCTION(BlueprintCallable)
	int32 GetAttackRange() const { return AttackRange; }
	UFUNCTION(BlueprintCallable)
	int32 GetDamageMin() const { return DamageMin; }
	UFUNCTION(BlueprintCallable)
	int32 GetDamageMax() const { return DamageMax; }
	UFUNCTION(BlueprintCallable)
	EUnitTurnOwner GetTurnOwnerType() const { return TurnOwnerType; }
	// Restituisce la posizione (x,y) dove si trova l'unita'
	UFUNCTION(BlueprintCallable, Category = "Unit|Grid")
	FIntPoint GetGridPosition() const { return UnitGridPosition; }
	UFUNCTION(BlueprintCallable)
	FGridCell GetCurrentCell() const;

	// Funzione che permette alla Tile sotto il player di illuminarsi
	UFUNCTION()
	void ToggleHighlightFunct(bool SToggleHighlight);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EUnitTurnOwner TurnOwnerType = EUnitTurnOwner::None;	

	// --- STATI DEL TURNO ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsPlaced = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bHasMoved = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bHasAttacked = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsMoving = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|State")
	bool ToggleHighlight = false;
	virtual bool IsSniper() const { return false; }


	// --- QUERY DI STATO ---
	UFUNCTION(BlueprintCallable, Category = "Unit|State")
	bool CanMove() const { return !bHasMoved; }
	UFUNCTION(BlueprintCallable, Category = "Unit|State")
	bool CanAttack() const { return !bHasAttacked; }
	UFUNCTION(BlueprintCallable, Category = "Unit|State")
	bool IsMoving() const { return !bIsMoving; }
	UFUNCTION(BlueprintCallable, Category = "Unit|State")
	bool IsDead() const { return bIsDead; }


	UFUNCTION(BlueprintCallable, Category = "Unit|Turn")
	void SetGridPosition(int32 X, int32 Y);



	// --- AZIONI/FUNZIONI DI GIOCO ---
	// --- MOVIMENTO ---
	// Muove l'unità verso le coordinate di destinazione, dopo la validazione generale.
	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	virtual void Move(int32 TargetX, int32 TargetY);

	//movimento AI
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMovementFinished OnMovementFinished;

	// rende possibile la visione del movimento e aspetta
	UFUNCTION(BlueprintCallable, Category = "Unit|Turn")
	void VisibileActionMoving(const TArray<FIntPoint>& NewPath);

	// --- Dati per il movimento visivo delle unita' ---
	UPROPERTY(EditAnywhere, Category = "Movement")
	float ZOffset = 90.0f; // Regola questo valore in base alla dimensione del tuo modello
	// Array che memorizza i punti del percorso rimasti da percorrere
	TArray<FVector> CurrentPathWorldPoints;
	int32 PathIndex = 0;
	// Velocità di movimento dell'unità
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MovementSpeed = 500.f;

	// --- ATTACCO ---
	// Verifica le condizioni ed esegue l'attacco contro un'altra unità. 
	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	virtual int32 Attack(ATBGBaseUnit* TargetUnit);

	// Sottrae HP. Se scendono <= 0, chiama OnDeath().
	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	virtual void ApplyTakenDamage(int32 Amount);

	// MORTE--------
	// Gestisce la morte dell'unità, rimuovendola dalla griglia e notificando la GameMode. 
	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	virtual void OnDeath();

	// Resetta lo stato delle AZIONI all'inizio del turno.
	UFUNCTION(BlueprintCallable, Category = "Unit|Turn")
	virtual void ResetTurnState();

	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	virtual void RespawnReset();

	// memorizza startingGridPosition
	UFUNCTION(BlueprintCallable, Category = "Unit|Actions")
	void ConfirmStartingPosition();


	// La mesh dell'unità
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit | Visual")
	UStaticMeshComponent* UnitMesh;

	// Materiale per l'unità della squadra Umana
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit | Visual")
	UMaterialInterface* HumanMaterial;

	// Materiale per l'unità della squadra AI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit | Visual")
	UMaterialInterface* AIMaterial;

	void UpdateTeamVisuals();
	
	UPROPERTY()
	AGridMapManager* GMM;
};
