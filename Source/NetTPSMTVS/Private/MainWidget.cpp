// Fill out your copyright notice in the Description page of Project Settings.


#include "MainWidget.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "NetPlayerController.h"
#include "NetTPSGameInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Components/TextBlock.h"

void UMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	btn_retry->OnClicked.AddDynamic(this , &UMainWidget::OnRetry);
	btn_exit->OnClicked.AddDynamic(this , &UMainWidget::OnExit);
}

// 플레이어가 태어날 때
void UMainWidget::InitBulletUI(int32 maxBulletCount)
{
	RemoveAllBulletUI();
	// 총알UI를 maxBulletCount만큼 채우고싶다.
	for ( int32 i = 0; i < maxBulletCount; i++ )
	{
		AddBulletUI();
	}
}

void UMainWidget::AddBulletUI()
{
	auto* bulletUI = CreateWidget(GetWorld() , BulletUIFactory);
	BulletPanel->AddChildToUniformGrid(bulletUI , 0 , BulletPanel->GetChildrenCount());
}

// 총을 쐈다면 호출
void UMainWidget::RemoveBulletUI()
{
	if ( BulletPanel->GetChildrenCount() <= 0 )
		return;

	BulletPanel->RemoveChildAt(BulletPanel->GetChildrenCount() - 1);
}

void UMainWidget::RemoveAllBulletUI()
{
	for ( UWidget* bulletWidget : BulletPanel->GetAllChildren() )
	{
		BulletPanel->RemoveChild(bulletWidget);
	}
}

void UMainWidget::SetActivePistolUI(bool value)
{
	if ( value )
	{
		ImageCrosshair->SetVisibility(ESlateVisibility::Visible);
		BulletPanel->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		ImageCrosshair->SetVisibility(ESlateVisibility::Hidden);
		BulletPanel->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMainWidget::PlayDamageAnimation()
{
	if ( DamageAnim )
	{
		PlayAnimation(DamageAnim);
	}
}

void UMainWidget::OnRetry()
{
	// 게임종료 UI 안보이도록 처리
	GameoverUI->SetVisibility(ESlateVisibility::Hidden);

	auto* pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
	if ( pc )
	{
		// 마우스커서도 안보이도록 처리
		pc->SetShowMouseCursor(false);
		// 리스폰 요청
		//pc->ServerRPCRespawnPlayer();
		// 관전자 등장
		pc->ServerRPCChangeToSpectator();
	}

}

void UMainWidget::OnExit()
{
	// 방에서 퇴장하고 싶다.
	auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
	if ( gi )
	{
		gi->ExitSession();
	}
}

void UMainWidget::NativeTick(const FGeometry& MyGeometry , float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. 다른 플레이어들의 정보를 알고싶다.
	TArray<TObjectPtr<APlayerState>> users = GetWorld()->GetGameState()->PlayerArray;

	// 2. 플레이어들의 이름을 다 모아서 
	FString names;
	for( APlayerState* user : users )
	{
		names.Append(FString::Printf(TEXT("%s\n"), *user->GetPlayerName()));
	}
	// 3. 출력하고싶다.
	txt_users->SetText(FText::FromString(names));
}
