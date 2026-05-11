// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "TBGPlayerController.generated.h"

class ATBGBaseUnit;
class AGridMapManager;
class UTBGUserWidget;
class ATBGTower;
class ATile;

// usata per semplicita' per alcune cose riguardati passaggio di unita'
UENUM(BlueprintType)
enum class EUnitType : uint8
{
    None,
    Brawler,
    Sniper
};

UCLASS()
class DURANTEDAVIDE_TBG_API ATBGPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATBGPlayerController();

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

public:
    // Chiamata da GameMode durante la fase di piazzamento
    void OnPlacementTurnStarted();

    bool bIsPlacingUnit = false;

    UFUNCTION(Client, Reliable)
    void Client_OnPlacementTurnStarted();

    // ----------------------------------------------------------
    // STATO DELLA SELEZIONE
    // Lo stato è derivato direttamente dai flag dell'unità
    // (bHasMoved, bHasAttacked) senza enum ridondanti.
    // ----------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LeftClickAction;


    // Unità attualmente selezionata (nullptr se nessuna)
    UPROPERTY(BlueprintReadOnly)
    ATBGBaseUnit* SelectedUnitClass = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<ATBGBaseUnit> SelectedUnitClassForPlacement; // classe scelta per piazzamento

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    TSubclassOf<ATBGBaseUnit> BrawlerPlacementClass;

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    TSubclassOf<ATBGBaseUnit> SniperPlacementClass;

    UPROPERTY(BlueprintReadWrite)
    EUnitType SelectedUnitType = EUnitType::None;

    UFUNCTION(BlueprintCallable)
    void SelectBrawlerForPlacement();

    UFUNCTION(BlueprintCallable)
    void SelectSniperForPlacement();
    // per hud
    void ShowPreviewStats();
    // per hud
    void ShowUnitStatsFromUnit(ATBGBaseUnit* Unit);

    UFUNCTION(BlueprintCallable)
    void RequestEndTurn();

private:
    // Riferimento cached al GridMapManager
    UPROPERTY()
    AGridMapManager* Grid;

    // ----------------------------------------------------------
    // INPUT HANDLERS
    // ----------------------------------------------------------

    /** Click sinistro: selezione, movimento, attacco, piazzamento */
    void OnLeftClick();

    void OnRightClick();

    // ----------------------------------------------------------
    // LOGICA DI GIOCO
    // ----------------------------------------------------------

    /**
     * Seleziona l'unità cliccata se appartiene al Player.
     * Se la stessa unità è già selezionata, la deseleziona.
     */
    void SelectUnit(ATBGBaseUnit* Unit);

    /**
     * Tenta di muovere l'unità selezionata verso la Tile
     * cliccata. Usa Pathfinder per calcolare il percorso e
     * verifica che il costo totale rispetti il MovementRange.
     * Aggiorna la griglia logica e la posizione fisica.
     * notifica GameMode tramite CheckPlayerTurnEnd().
     */
    void MoveUnit(ATile* Tile);

    /**
     * Tenta di attaccare l'unità nemica cliccata.
     * Verifica distanza, range e livello di elevazione.
     * Calcola il danno tramite DamageCalculator.
     * Applica eventuale contrattacco allo Sniper.
     * Notifica GameMode tramite CheckPlayerTurnEnd().
     */
    void AttemptAttack(ATBGBaseUnit* Target);

    // ----------------------------------------------------------
    // UTILITY
    // ----------------------------------------------------------

    /**
     * Esegue un line trace dalla posizione del cursore e
     * ritorna l'attore colpito in OutActor.
     * Ritorna true se ha colpito qualcosa di valido.
     */
    bool GetInteractableUnderCursor(AActor*& OutActor) const;
};