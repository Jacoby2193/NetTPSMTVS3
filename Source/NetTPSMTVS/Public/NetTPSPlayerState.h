// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetTPSPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NETTPSMTVS_API ANetTPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	UFUNCTION(Server, Reliable)
	void ServerRPCSetPlayerName(const FString& newName);

	
};
