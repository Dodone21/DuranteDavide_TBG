// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGBaseUnit.h"
#include "TBGUnitSniper.generated.h"

/**
 * 
 */
UCLASS()
class DURANTEDAVIDE_TBG_API ATBGUnitSniper : public ATBGBaseUnit
{
	GENERATED_BODY()

public:
	ATBGUnitSniper();

protected:
	virtual void BeginPlay() override;

	virtual bool IsSniper() const { return true; }
};
