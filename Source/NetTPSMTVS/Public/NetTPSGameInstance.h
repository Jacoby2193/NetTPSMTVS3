// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NetTPSGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class NETTPSMTVS_API UNetTPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// 온라인 세션 인터페이스를 기억하고싶다.
	IOnlineSessionPtr SessionInterface;
};
