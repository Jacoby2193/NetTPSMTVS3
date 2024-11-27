// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NetTPSGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString roomName;
	UPROPERTY(BlueprintReadOnly)
	FString hostName;
	UPROPERTY(BlueprintReadOnly)
	int32 maxPlayerCount;
	UPROPERTY(BlueprintReadOnly)
	int32 currentPlayerCount;
	UPROPERTY(BlueprintReadOnly)
	int32 pingMS;

	int32 index;

	FString ToString()
	{
		return FString::Printf(TEXT("%d)[%s][%s] (%d / %d) -> %dms"), index, *roomName, *hostName, currentPlayerCount, maxPlayerCount, pingMS);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSearchSignature , const struct FRoomInfo& , info);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindSignature , bool, value);

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

	// 찾을 방의 목록
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	// 방찾기 요청
	void FindOtherSessions();
	// 방찾기 응답
	void OnMyFindSessionsCompleteDelegates(bool bWasSuccessful);

	FSearchSignature OnSearchSignatureCompleteDelegate;

	// 찾기를 위한 델리게이트...
	FFindSignature OnFindSignatureCompleteDelegate;

	// 방입장 요청
	void MyJoinSession(int32 index);
	// 방입장 응답
	void OnMyJoinSessionComplete(FName SessionName , EOnJoinSessionCompleteResult::Type EOnJoinSessionCompleteResult);



	// 방퇴장 요청 -> UI에서 호출
	void ExitSession();

	UFUNCTION(Server, Reliable)
	void ServerRPCExitSession();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCExitSession();
	// 방퇴장 응답

	void OnMyDestroySessionComplete(FName SessionName , bool bWasSuccessful);

	// 스팀으로 한글이름 방을 만들어서 조회하면 한글이 깨지는 이슈발생!!
	// Base64 인코딩으로 해결하고자함!
	FString StringBase64Encode(const FString& str);

	FString StringBase64Decode(const FString& str);


	bool bTypeA;

};
