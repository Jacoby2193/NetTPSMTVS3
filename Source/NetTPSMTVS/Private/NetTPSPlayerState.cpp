// Fill out your copyright notice in the Description page of Project Settings.


#include "NetTPSPlayerState.h"
#include "NetTPSGameInstance.h"

void ANetTPSPlayerState::BeginPlay()
{
	Super::BeginPlay();

	auto* pc = GetWorld()->GetFirstPlayerController();
	if (pc && pc->IsLocalController())
	{
		auto* gi = Cast<UNetTPSGameInstance>(GetWorld()->GetGameInstance());
		if ( gi )
		{
			ServerRPCSetPlayerName(gi->MySessionName);
		}
	}
}

void ANetTPSPlayerState::ServerRPCSetPlayerName_Implementation(const FString& newName)
{
	SetPlayerName(newName);
}
