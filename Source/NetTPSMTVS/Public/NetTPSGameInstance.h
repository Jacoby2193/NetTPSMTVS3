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

	FString MySessionName = TEXT("JacobYi");

	// 방생성 요청
	void CreateMySession(FString roomName, int32 playerCount);
	// 방생성 응답
	void OnMyCreateSessionComplete(FName SessionName , bool bWasSuccessful);
};
