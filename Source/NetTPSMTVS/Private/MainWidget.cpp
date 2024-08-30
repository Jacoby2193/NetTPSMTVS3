// Fill out your copyright notice in the Description page of Project Settings.


#include "MainWidget.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"

// 플레이어가 태어날 때
void UMainWidget::InitBulletUI(int32 maxBulletCount)
{
	// 총알UI를 maxBulletCount만큼 채우고싶다.
	for(int32 i=0 ; i< maxBulletCount  ; i++)
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

void UMainWidget::SetActivePistolUI(bool value)
{
	if (value)
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
