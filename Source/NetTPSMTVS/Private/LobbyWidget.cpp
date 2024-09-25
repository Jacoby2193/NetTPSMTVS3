// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "NetTPSGameInstance.h"
#include "Components/EditableText.h"
#include "Components/WidgetSwitcher.h"
#include "SessionSlotWidget.h"
#include "Components/ScrollBox.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 게임인스턴스를 가져와서
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	// OnSearchSignatureCompleteDelegate에 AddSessionSlotWidget을 연결하고싶다.
	gi->OnSearchSignatureCompleteDelegate.AddDynamic(this , &ULobbyWidget::AddSessionSlotWidget);
	gi->OnFindSignatureCompleteDelegate.AddDynamic(this , &ULobbyWidget::SetFindActive);


	MENU_Button_GoCreateRoom->OnClicked.AddDynamic(this , &ULobbyWidget::MENU_OnClickGoCreateRoom);
	MENU_Button_GoFindSessions->OnClicked.AddDynamic(this , &ULobbyWidget::MENU_OnClickGoFindSessions);

	CR_Button_GoMenu->OnClicked.AddDynamic(this , &ULobbyWidget::OnClickGoMenu);
	FS_Button_GoMenu->OnClicked.AddDynamic(this , &ULobbyWidget::OnClickGoMenu);

	CR_Button_CreateRoom->OnClicked.AddDynamic(this , &ULobbyWidget::CR_OnClickCreateRoom);
	CR_Slider_PlayerCount->OnValueChanged.AddDynamic(this , &ULobbyWidget::CR_OnChangeSliderPlayerCount);

	FS_Button_FindSessions->OnClicked.AddDynamic(this , &ULobbyWidget::FS_OnClickFindSessions);

	CR_Slider_PlayerCount->SetValue(2);
	SetFindActive(false);
}

void ULobbyWidget::OnClickGoMenu()
{
	LobbyWidgetSwitcher->SetActiveWidgetIndex(0);
}

void ULobbyWidget::MENU_OnClickGoCreateRoom()
{
	// MENU_Edit_SessionName의 내용을 UNetTPSGameInstance의 MySessionName 에 반영하고싶다.
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	FString newSessionName = MENU_Edit_SessionName->GetText().ToString();
	if ( gi && false == newSessionName.IsEmpty() )
	{
		gi->MySessionName = MENU_Edit_SessionName->GetText().ToString();
	}
	LobbyWidgetSwitcher->SetActiveWidgetIndex(1);
}

void ULobbyWidget::MENU_OnClickGoFindSessions()
{
	// MENU_Edit_SessionName의 내용을 UNetTPSGameInstance의 MySessionName 에 반영하고싶다.
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	FString newSessionName = MENU_Edit_SessionName->GetText().ToString();
	if ( gi && false == newSessionName.IsEmpty() )
	{
		gi->MySessionName = MENU_Edit_SessionName->GetText().ToString();
	}
	LobbyWidgetSwitcher->SetActiveWidgetIndex(2);

	FS_OnClickFindSessions();
}

void ULobbyWidget::CR_OnClickCreateRoom()
{
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	FString roomName = CR_Edit_RoomName->GetText().ToString();

	// roomName이 기재되지 않거나 공백이라면 방생성을 하지 않고싶다.
	roomName = roomName.TrimStartAndEnd();
	if ( roomName.IsEmpty() )
	{
		return;
	}

	int32 count = (int32)CR_Slider_PlayerCount->GetValue();
	gi->CreateMySession(roomName , count);
}

void ULobbyWidget::CR_OnChangeSliderPlayerCount(float value)
{
	// 슬라이더의 값이 변경되면 CR_Text_PlayerCount에 값을 반영하고 싶다.
	CR_Text_PlayerCount->SetText(FText::AsNumber(value));

}

void ULobbyWidget::FS_OnClickFindSessions()
{
	// 기존의 방 목록을 삭제하고
	FS_ScrollBox->ClearChildren();

	// 방찾기를 요청하고싶다.
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	if ( gi )
	{
		gi->FindOtherSessions();
	}
}

void ULobbyWidget::AddSessionSlotWidget(const struct FRoomInfo& info)
{
	// 슬롯을 생성해서 
	auto* slot = CreateWidget<USessionSlotWidget>(this , SessionSlotWidgetFactory);

	slot->UpdateInfo(info);
	// FS_ScrollBox에 추가 하고싶다.

	FS_ScrollBox->AddChild(slot);
}

void ULobbyWidget::SetFindActive(bool value)
{
	// 찾기를 시도하면 Find텍스트를 보이게, 버튼 비활성 하고싶다.
	// 찾기가 끝나면 Find텍스트 안보이게, 버튼 활성 하고싶다.
	if ( value )
	{
		// 찾기 시도
		FS_Text_Finding->SetVisibility(ESlateVisibility::Visible);
		FS_Button_FindSessions->SetIsEnabled(false);
	}
	else
	{
		// 찾기 끝
		FS_Text_Finding->SetVisibility(ESlateVisibility::Hidden);
		FS_Button_FindSessions->SetIsEnabled(true);
	}

}
