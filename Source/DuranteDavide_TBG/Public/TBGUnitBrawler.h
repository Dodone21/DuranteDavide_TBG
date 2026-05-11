// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGBaseUnit.h"
#include "TBGUnitBrawler.generated.h"

/**
 * 
 */
UCLASS()
class DURANTEDAVIDE_TBG_API ATBGUnitBrawler : public ATBGBaseUnit
{
	GENERATED_BODY()

public:
	ATBGUnitBrawler();

protected:
	virtual void BeginPlay() override;
	
	virtual bool IsSniper() const { return false; }
};
