// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetActor.generated.h"

UCLASS()
class NETTPSMTVS_API ANetActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANetActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FTimerHandle handle;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// StaticMesh(Sphere)를 추가하고 싶다.
	UPROPERTY(EditDefaultsOnly)
	class UStaticMeshComponent* MeshComp;

	void PrintNetLog();

	// Owner 검출 영역
	UPROPERTY(EditDefaultsOnly)
	float SearchDistance = 200;

	// Owner 설정
	void FindOwner();

	// 회전 값 동기화 변수
	UPROPERTY(ReplicatedUsing = OnRep_RotYaw)
	float RotYaw;

	UFUNCTION()
	void OnRep_RotYaw();

	UPROPERTY()
	class UMaterialInstanceDynamic* Mat;

	// 재질에 동기화될 색상
	UPROPERTY(ReplicatedUsing = OnRep_ChangeMatColor)
	FLinearColor MatColor;
	
	UFUNCTION()
	void OnRep_ChangeMatColor();

	UFUNCTION(Server, Reliable)
	void ServerRPC_ChangeColor(const FLinearColor newColor);

	UFUNCTION(Client, Reliable)
	void ClientRPC_ChangeColor(const FLinearColor newColor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_ChangeColor(const FLinearColor newColor);
};
