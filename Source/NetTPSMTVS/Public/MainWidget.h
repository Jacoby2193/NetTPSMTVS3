// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainWidget.generated.h"

/**
 *
 */
UCLASS()
class NETTPSMTVS_API UMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	// Crosshair이미지를 바인딩 하고싶다.
	UPROPERTY(meta = (BindWidget))
	class UImage* ImageCrosshair;

	UPROPERTY(meta = (BindWidget))
	class UUniformGridPanel* BulletPanel;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> BulletUIFactory;

	// 총알UI 초기설정 (10발)
	void InitBulletUI(int32 maxBulletCount);
	// 총알UI 추가
	void AddBulletUI();
	// 총알UI 차감
	void RemoveBulletUI();

	void RemoveAllBulletUI();

	// 주인공이 태어날 때 UMainWidget을 만들어서 가지고 있고싶다.

	// Crosshair이미지를 켜고 끄는 기능을 만들고
	void SetActivePistolUI(bool value);
	// 총을 잡으면 켜고
	// 총을 놓으면 끄고싶다.

	UPROPERTY(EditDefaultsOnly , BlueprintReadWrite , Category = HP)
	float HP = 1.0f;

	UPROPERTY(meta=(BindWidget))
	class UProgressBar* HealthBar;

	// DamageUI 애니메이션
	UPROPERTY(EditDefaultsOnly, meta = (BindWidgetAnim), Transient, Category = MySettings)
	class UWidgetAnimation* DamageAnim;

	void PlayDamageAnimation();

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UHorizontalBox* GameoverUI;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UButton* btn_retry;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UButton* btn_exit;

	UFUNCTION()
	void OnRetry();

	UFUNCTION()
	void OnExit();

	// 상대방 플레이어들의 이름을 우측에 보이게 하고싶다.
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* txt_users;

	virtual void NativeTick(const FGeometry& MyGeometry , float InDeltaTime) override;


	// 채팅 ============
	// 1. send 버튼을 누르면 서버로 edit_input의 내용을 ServerRPC로 보내고 싶다.
	// 주인공 -> ServerRPC를 하면 MulticastRPC를 해서 다시 내용을 전달 받을 예정이다.
	// 2. 전달받은 message를 ChatWidget을 만들어서 scroll_msgList에 추가하고싶다.

	UPROPERTY(meta=(BindWidget))
	class UScrollBox* scroll_msgList;

	UPROPERTY(meta=(BindWidget))
	class UEditableText* edit_input;

	UPROPERTY(meta=(BindWidget))
	class UButton* btn_send;

	UFUNCTION()
	void OnMySend();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UChatWidget> ChatWidgetFactory;

	void AddChatMessage(const FString& msg);


};
