// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerController.h"
#include "NetTPSMTVSGameMode.h"

void ANetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GM = Cast<ANetTPSMTVSGameMode>(GetWorld()->GetAuthGameMode());
	}
}

void ANetPlayerController::ServerRPCRespawnPlayer_Implementation()
{
	
	auto* player = GetPawn();
	UnPossess();
	if ( player ) 
	{
		player->Destroy();
	}

	GM->RestartPlayer(this);

}
