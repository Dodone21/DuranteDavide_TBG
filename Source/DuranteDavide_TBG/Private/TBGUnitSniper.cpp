// Fill out your copyright notice in the Description page of Project Settings.


#include "TBGUnitSniper.h"

ATBGUnitSniper::ATBGUnitSniper()
{
	MaxHP = 20;
	MovementRange = 4;
	AttackRange = 10;
	DamageMin = 4;
	DamageMax = 8;
}

void ATBGUnitSniper::BeginPlay()
{
	Super::BeginPlay();
}

