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

public:	

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// StaticMesh(Sphere)를 추가하고 싶다.
	UPROPERTY(EditDefaultsOnly)
	class UStaticMeshComponent* MeshComp;

	void PrintNetLog();

	// Owner 검출 영역
	UPROPERTY(EditDefaultsOnly)
	float searchDistance = 200;
};
