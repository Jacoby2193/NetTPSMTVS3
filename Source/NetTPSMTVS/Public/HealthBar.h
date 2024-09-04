﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBar.generated.h"

/**
 * 
 */
UCLASS()
class NETTPSMTVS_API UHealthBar : public UUserWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly , BlueprintReadWrite , Category = HP)
	float HP = 1.0f;
};