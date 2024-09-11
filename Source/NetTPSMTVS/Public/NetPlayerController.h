// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NETTPSMTVS_API ANetPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class ANetTPSMTVSGameMode* GM;

	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerRPCRespawnPlayer();

	// MainWidget을 생성해서 기억하고싶다.
	// 기억하는 위치를 PlayerController쪽에 만들어서 Character를 다시 만들어도 UI가 유지되도록 하고싶다.
	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSubclassOf<class UMainWidget> MainUIWidget;
	
	// MainUIWidget으로부터 만들어진 인스턴스
	UPROPERTY()
	class UMainWidget* MainUI;

	UFUNCTION(Server, Reliable)
	void ServerRPCChangeToSpectator();

};
