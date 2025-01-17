﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "NetTPSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "NetTPSMTVS.h"
#include "Online/OnlineSessionNames.h"
#include "string"

void UNetTPSGameInstance::Init()
{
	Super::Init();

	if ( auto* subSystem = IOnlineSubsystem::Get() )
	{
		SessionInterface = subSystem->GetSessionInterface();

		// 방생성 요청 -> 응답
		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this , &UNetTPSGameInstance::OnMyCreateSessionComplete);

		// 방찾기 응답
		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this , &UNetTPSGameInstance::OnMyFindSessionsCompleteDelegates);

		// 방입장 응답
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this , &UNetTPSGameInstance::OnMyJoinSessionComplete);

		// 방퇴장 응답
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this , &UNetTPSGameInstance::OnMyDestroySessionComplete);
	}

	//PRINTLOG(TEXT("Network Start!!"));
	//FTimerHandle handle;
	//GetWorld()->GetTimerManager().SetTimer(handle , [&]() {
	//	/*CreateMySession(MySessionName , 10);*/
	//	FindOtherSessions();
	//	} , 3 , false);

}

void UNetTPSGameInstance::CreateMySession(FString roomName , int32 playerCount)
{
	FOnlineSessionSettings settings;

	// 1. 전용서버를 사용하는가?
	settings.bIsDedicated = false;

	// 2. 랜선(Lan)으로 매치하는가?
	FName subsysName = IOnlineSubsystem::Get()->GetSubsystemName();
	settings.bIsLANMatch = subsysName == "NULL";

	// 3. 매칭이 공개(true)혹은 비공개(false, 초대를 통해서 매칭)
	settings.bShouldAdvertise = true;

	// 4. 유저의 상태 정보(온라인/자리비움/등등) 사용 여부
	settings.bUsesPresence = true;
	
	// 5. 중간에 난입 가능한가?
	settings.bAllowJoinViaPresence = true;
	settings.bAllowJoinInProgress = true;

	// 6. 최대 인원수
	settings.NumPublicConnections = playerCount;

	// 7. 커스텀 정보
	
	settings.Set(FName("ROOM_NAME") , StringBase64Encode(roomName) , EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	settings.Set(FName("HOST_NAME") , StringBase64Encode(MySessionName), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	FUniqueNetIdPtr netID = GetWorld()->GetFirstLocalPlayerFromController()->GetUniqueNetIdForPlatformUser().GetUniqueNetId();

	SessionInterface->CreateSession(*netID , FName(MySessionName), settings);

	PRINTLOG(TEXT("Create Session Start roomNamd : %s / hostName : %s") , *roomName , *MySessionName);
}

void UNetTPSGameInstance::OnMyCreateSessionComplete(FName SessionName , bool bWasSuccessful)
{
	if ( bWasSuccessful )
	{
		PRINTLOG(TEXT("OnMyCreateSessionComplete is Success~~~~~"));

		// 서버가 여행을 떠나고싶다.
		GetWorld()->ServerTravel(TEXT("/Game/NetTPS/Maps/BattleMap?listen"));
	}
	else
	{
		PRINTLOG(TEXT("OnMyCreateSessionComplete is Failed!!!"));
	}
}

void UNetTPSGameInstance::FindOtherSessions()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch);

	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	SessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	SessionSearch->MaxSearchResults = 40;

	SessionInterface->FindSessions(0 , SessionSearch.ToSharedRef());
	// 찾기 UI를 활성화 하고싶다.
	if ( OnFindSignatureCompleteDelegate.IsBound())
	{
		OnFindSignatureCompleteDelegate.Broadcast(true);
	}
}

void UNetTPSGameInstance::OnMyFindSessionsCompleteDelegates(bool bWasSuccessful)
{
	if ( bWasSuccessful )
	{
		TArray<FOnlineSessionSearchResult> results = SessionSearch->SearchResults;

		for (int32 i=0 ; i< results.Num() ; i++)
		{
			FOnlineSessionSearchResult ret = results[i];
			if (false == ret.IsValid())
			{
				continue;
			}

			FRoomInfo roomInfo;
			roomInfo.index = i;

			// 방이름
			FString roomName;
			ret.Session.SessionSettings.Get(FName("ROOM_NAME") , roomName);
			roomInfo.roomName = StringBase64Decode(roomName);
			// 호스트이름
			FString hostName;
			ret.Session.SessionSettings.Get(FName("HOST_NAME") , hostName);
			roomInfo.hostName = StringBase64Decode(hostName);
			// 최대 플레이어 수
			roomInfo.maxPlayerCount = ret.Session.SessionSettings.NumPublicConnections;
			// 입장한 플레이어 수(최대 - 남은)
			roomInfo.currentPlayerCount = roomInfo.maxPlayerCount - ret.Session.NumOpenPublicConnections;
			// 핑 정보
			roomInfo.pingMS = ret.PingInMs;

			if ( OnSearchSignatureCompleteDelegate.IsBound() )
				OnSearchSignatureCompleteDelegate.Broadcast(roomInfo);

			PRINTLOG(TEXT("%s") , *roomInfo.ToString());
		}
	}
	else
	{
		PRINTLOG(TEXT("OnMyFindSessionsCompleteDelegates is Failed!!!"));
	}

	// 찾기 UI를 비활성화 하고싶다.
	if ( OnFindSignatureCompleteDelegate.IsBound() )
	{
		OnFindSignatureCompleteDelegate.Broadcast(false);
	}
}

void UNetTPSGameInstance::MyJoinSession(int32 index)
{
	auto result = SessionSearch->SearchResults[index];
	SessionInterface->JoinSession(0 , FName(MySessionName) , result);
}

void UNetTPSGameInstance::OnMyJoinSessionComplete(FName SessionName , EOnJoinSessionCompleteResult::Type EOnJoinSessionCompleteResult)
{
	if ( EOnJoinSessionCompleteResult::Success == EOnJoinSessionCompleteResult )
	{
		// 서버가 있는 레벨로 여행을 떠나고 싶다.
		auto* pc = GetWorld()->GetFirstPlayerController();

		FString url;
		SessionInterface->GetResolvedConnectString(SessionName, url);
		if (false == url.IsEmpty())
		{
			pc->ClientTravel(url , ETravelType::TRAVEL_Absolute);
		}
	}
}

void UNetTPSGameInstance::ExitSession()
{
	ServerRPCExitSession();
}

void UNetTPSGameInstance::ServerRPCExitSession_Implementation()
{
	MulticastRPCExitSession();
}

void UNetTPSGameInstance::MulticastRPCExitSession_Implementation()
{
	// 방퇴장 요청
	SessionInterface->DestroySession(FName(MySessionName));
}

void UNetTPSGameInstance::OnMyDestroySessionComplete(FName SessionName , bool bWasSuccessful)
{
	if ( bWasSuccessful )
	{
		// 클라이언트가 로비로 여행을 가고싶다.
		auto* pc = GetWorld()->GetFirstPlayerController();
		pc->ClientTravel(TEXT("/Game/NetTPS/Maps/LobbyMap"), ETravelType::TRAVEL_Absolute);
	}
}

FString UNetTPSGameInstance::StringBase64Encode(const FString& str)
{
	// Set 할 때 : FString -> UTF8 (std::string) -> TArray<uint8> -> base64 로 Encode
	std::string utf8String = TCHAR_TO_UTF8(*str);
	TArray<uint8> arrayData = TArray<uint8>((uint8*)(utf8String.c_str()) ,
	utf8String.length());
	return FBase64::Encode(arrayData);

}

FString UNetTPSGameInstance::StringBase64Decode(const FString& str)
{
	// Get 할 때 : base64 로 Decode -> TArray<uint8> -> TCHAR
	TArray<uint8> arrayData;
	FBase64::Decode(str , arrayData);
	std::string ut8String((char*)(arrayData.GetData()) , arrayData.Num());
	return UTF8_TO_TCHAR(ut8String.c_str());

}
