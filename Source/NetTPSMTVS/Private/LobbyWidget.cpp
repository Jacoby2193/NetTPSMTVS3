// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "NetTPSGameInstance.h"
#include "Components/EditableText.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CR_Button_CreateRoom->OnClicked.AddDynamic(this, &ULobbyWidget::CR_OnClickCreateRoom);
	CR_Slider_PlayerCount->OnValueChanged.AddDynamic(this , &ULobbyWidget::CR_OnChangeSliderPlayerCount);

	CR_Slider_PlayerCount->SetValue(2);
}

void ULobbyWidget::CR_OnClickCreateRoom()
{
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	FString roomName = CR_Edit_RoomName->GetText().ToString();

	// roomName이 기재되지 않거나 공백이라면 방생성을 하지 않고싶다.
	roomName = roomName.TrimStartAndEnd();
	if (roomName.IsEmpty())
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
