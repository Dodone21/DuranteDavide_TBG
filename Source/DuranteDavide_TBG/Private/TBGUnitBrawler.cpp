// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGUnitBrawler.h"

ATBGUnitBrawler::ATBGUnitBrawler()
{
	MaxHP = 40;
	MovementRange = 6;
	AttackRange = 1;
	DamageMin = 1;
	DamageMax = 6;
}

void ATBGUnitBrawler::BeginPlay()
{
	Super::BeginPlay();
}
