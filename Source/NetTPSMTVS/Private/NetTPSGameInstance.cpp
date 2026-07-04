// Fill out your copyright notice in the Description page of Project Settings.


#include "NetTPSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "NetTPSMTVS.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "string"

void UNetTPSGameInstance::Init()
{
	Super::Init();

	// 방장이 방에서 퇴장하면 ConnectionLost 라는 네트워크 오류가 발생되는데 로비로 튕겨나도록 한다. 
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this , &UNetTPSGameInstance::OnMyNetworkFailure);
	}

	if (auto* subSystem = Online::GetSubsystem(GetWorld()))
	{
		SessionInterface = subSystem->GetSessionInterface();

		// 방생성 요청 -> 응답
		CreateSessionDelegateHandle = SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
			this , &UNetTPSGameInstance::OnMyCreateSessionComplete);

		// 방찾기 응답
		FindSessionsDelegateHandle = SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
			this , &UNetTPSGameInstance::OnMyFindSessionsCompleteDelegates);

		// 방입장 응답
		JoinSessionDelegateHandle = SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
			this , &UNetTPSGameInstance::OnMyJoinSessionComplete);

		// 방퇴장 응답
		DestroySessionDelegateHandle = SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
			this , &UNetTPSGameInstance::OnMyDestroySessionComplete);

		// 친구 초대 수락 델리게이트 바인딩
		InviteAcceptedDelegateHandle = SessionInterface->OnSessionUserInviteAcceptedDelegates.AddUObject(
			this , &UNetTPSGameInstance::OnMyInviteAccepted);
	}

	//PRINTLOG(TEXT("Network Start!!"));
	//FTimerHandle handle;
	//GetWorld()->GetTimerManager().SetTimer(handle , [&]() {
	//	/*CreateMySession(MySessionName , 10);*/
	//	FindOtherSessions();
	//	} , 3 , false);
}

void UNetTPSGameInstance::Shutdown()
{
	// SessionInterface가 유효한지 확인 후 핸들을 이용해 델리게이트를 제거합니다.
	if (SessionInterface.IsValid())
	{
		SessionInterface->OnCreateSessionCompleteDelegates.Remove(CreateSessionDelegateHandle);
		SessionInterface->OnFindSessionsCompleteDelegates.Remove(FindSessionsDelegateHandle);
		SessionInterface->OnJoinSessionCompleteDelegates.Remove(JoinSessionDelegateHandle);
		SessionInterface->OnDestroySessionCompleteDelegates.Remove(DestroySessionDelegateHandle);
		SessionInterface->OnSessionUserInviteAcceptedDelegates.Remove(InviteAcceptedDelegateHandle);
	}

	// GEngine 네트워크 실패 델리게이트도 제거 (필요시 핸들을 사용하거나 특정 객체 바인딩 전체 제거)
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UNetTPSGameInstance::CreateMySession(FString roomName , int32 playerCount)
{
	FOnlineSessionSettings settings;

	// 1. 전용서버를 사용하는가?
	settings.bIsDedicated = false;

	// 2. 랜선(Lan)으로 매치하는가?
	FName subsysName = Online::GetSubsystem(GetWorld())->GetSubsystemName();
	settings.bIsLANMatch = subsysName == "NULL";

	// 3. 매칭이 공개(true)혹은 비공개(false, 초대를 통해서 매칭)
	settings.bShouldAdvertise = true;

	// 4. 유저의 상태 정보(온라인/자리비움/등등) 사용 여부
	settings.bUsesPresence = true;
	settings.bUseLobbiesIfAvailable = true;
	
	// 5. 중간에 난입 가능한가?
	settings.bAllowJoinViaPresence = true;
	settings.bAllowJoinInProgress = true;

	// 6. 최대 인원수
	settings.NumPublicConnections = playerCount;

	// 7. 커스텀 정보

	settings.Set(FName("ROOM_NAME") , StringBase64Encode(roomName) ,
	             EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	settings.Set(FName("HOST_NAME") , StringBase64Encode(MySessionName) ,
	             EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	FUniqueNetIdPtr netID = GetWorld()->GetFirstLocalPlayerFromController()->GetUniqueNetIdForPlatformUser().
	                                    GetUniqueNetId();

	SessionInterface->CreateSession(*netID , FName(MySessionName) , settings);

	PRINTLOG(TEXT("Create Session Start roomNamd : %s / hostName : %s") , *roomName , *MySessionName);
}

void UNetTPSGameInstance::OnMyCreateSessionComplete(FName SessionName , bool bWasSuccessful)
{
	if (bWasSuccessful)
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

	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES , true , EOnlineComparisonOp::Equals);
	SessionSearch->bIsLanQuery = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL";
	SessionSearch->MaxSearchResults = 40;

	SessionInterface->FindSessions(0 , SessionSearch.ToSharedRef());
	// 찾기 UI를 활성화 하고싶다.
	if (OnFindSignatureCompleteDelegate.IsBound())
	{
		OnFindSignatureCompleteDelegate.Broadcast(true);
	}
}

void UNetTPSGameInstance::OnMyFindSessionsCompleteDelegates(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		TArray<FOnlineSessionSearchResult> results = SessionSearch->SearchResults;

		for (int32 i = 0; i < results.Num(); i++)
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

			if (OnSearchSignatureCompleteDelegate.IsBound())
				OnSearchSignatureCompleteDelegate.Broadcast(roomInfo);

			PRINTLOG(TEXT("%s") , *roomInfo.ToString());
		}
	}
	else
	{
		PRINTLOG(TEXT("OnMyFindSessionsCompleteDelegates is Failed!!!"));
	}

	// 찾기 UI를 비활성화 하고싶다.
	if (OnFindSignatureCompleteDelegate.IsBound())
	{
		OnFindSignatureCompleteDelegate.Broadcast(false);
	}
}

void UNetTPSGameInstance::MyJoinSession(int32 index)
{
	auto result = SessionSearch->SearchResults[index];
	SessionInterface->JoinSession(0 , FName(MySessionName) , result);
}

void UNetTPSGameInstance::OnMyJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (EOnJoinSessionCompleteResult::Success == Result)
	{
		// 세션 인터페이스 유효성 검사 (안전 장치)
		if (false == SessionInterface.IsValid())
		{
			PRINTLOG(TEXT("세션 인터페이스가 유효하지 않습니다."));
			return;
		}

		// 1. 세션(전용 서버 또는 리슨 서버)의 실제 IP 주소와 포트를 가져옵니다.
		FString url;
		SessionInterface->GetResolvedConnectString(SessionName , url);
		
		if (false == url.IsEmpty())
		{
			if (auto* pc = GetWorld()->GetFirstPlayerController())
			{
				// 2. 해당 서버 주소로 클라이언트를 강제 이동(접속)시킵니다.
				pc->ClientTravel(url , ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UNetTPSGameInstance::ExitSession()
{
	// 방퇴장 요청
	SessionInterface->DestroySession(FName(MySessionName));
}

void UNetTPSGameInstance::OnMyDestroySessionComplete(FName SessionName , bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// 클라이언트가 로비로 여행을 가고싶다.
		auto* pc = GetWorld()->GetFirstPlayerController();
		pc->ClientTravel(TEXT("/Game/NetTPS/Maps/LobbyMap") , ETravelType::TRAVEL_Absolute);
	}
}

void UNetTPSGameInstance::OnMyNetworkFailure(UWorld* world , UNetDriver* netDriver , ENetworkFailure::Type failType ,
                                             const FString& erorrMsg)
{
	if (failType == ENetworkFailure::ConnectionLost)
	{
		ExitSession();
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

void UNetTPSGameInstance::OnMyInviteAccepted(const bool bWasSuccessful , const int32 ControllerId ,
                                             TSharedPtr<const FUniqueNetId> UserId ,
                                             const FOnlineSessionSearchResult& InviteResult)
{
	// ControllerId는 초대를 수락한 유저의 로컬 플레이어 인덱스(Local Player Index)입니다.
	// PC 기반의 스팀 게임이라면 이 값은 항상 0이 들어옵니다.
	// 하나의 PC에서 여러 명이 동시에 플레이하는 분할 화면(Split-screen) 게임이 아닌 이상,
	// 주 플레이어의 로컬 인덱스는 무조건 0이기 때문입니다.

	// UserId는 초대를 수락한 유저의 고유 네트워크 ID(Unique Network ID)입니다.
	// 현재 Steam 온라인 서브시스템을 사용 중이시므로,
	// 이 객체 내부에는 유저의 스팀 고유 식별 번호 (Steam64 ID)가 들어있습니다.
	// Steam ID와 같은 값입니다.
	if (bWasSuccessful)
	{
		// 만약 친구의 스팀 닉네임을 가져오려면 
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			// 1. 나를 초대한 친구(세션 방장)의 닉네임 가져오기
			// 굳이 친구 인터페이스를 거치지 않아도, InviteResult 내부에 방장의 이름이 이미 들어있습니다.
			FString FriendDisplayName = InviteResult.Session.OwningUserName;
			PRINTLOG(TEXT("나를 초대한 친구의 닉네임: %s") , *FriendDisplayName);

			/*
			// 참고: 만약 닉네임뿐만 아니라 친구의 상세 정보(상태 등) 객체가 필요하다면
			//UserId가 아닌 InviteResult.Session.OwningUserId 를 넣어 검색해야 합니다.
			
			IOnlineFriendsPtr FriendsInterface = OnlineSub->GetFriendsInterface();
			if (FriendsInterface.IsValid() && InviteResult.Session.OwningUserId.IsValid())
			{
				TSharedPtr<FOnlineFriend> FriendInfo = FriendsInterface->GetFriend(
					ControllerId , *InviteResult.Session.OwningUserId , TEXT("default"));
				if (FriendInfo.IsValid())
				{
					// 친구의 닉네임을 가져옵니다.
					FString FriendDisplayName = FriendInfo->GetDisplayName();
					PRINTLOG(TEXT("나를 초대한 친구의 닉네임: %s") , *FriendDisplayName);
				}
			}
			*/
			

			// 2. 나의 닉네임 가져오기
			IOnlineIdentityPtr idInterface = OnlineSub->GetIdentityInterface();
			if (idInterface.IsValid())
			{
				FString MyNickName = idInterface->GetPlayerNickname(ControllerId);
				PRINTLOG(TEXT("나의 닉네임: %s") , *MyNickName);
			}
		}

		PRINTLOG(TEXT("스팀 친구 초대를 수락했습니다. 세션에 입장을 시도합니다."));

		// 전달받은 InviteResult(세션 정보)를 사용하여 즉시 조인
		SessionInterface->JoinSession(ControllerId , FName(MySessionName) , InviteResult);
	}
}
