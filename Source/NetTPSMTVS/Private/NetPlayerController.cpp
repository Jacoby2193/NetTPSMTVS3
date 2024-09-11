// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerController.h"
#include "NetTPSMTVSGameMode.h"
#include "GameFramework/SpectatorPawn.h"

void ANetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if ( HasAuthority() )
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

void ANetPlayerController::ServerRPCChangeToSpectator_Implementation()
{
	// 관전자가 플레이어의 위치에서 생성될 수 있도록 플레이어 정보를 가져오고싶다.
	APawn* player = GetPawn();
	if ( player )
	{
		// 관전자를 생성해서 Possess 하고싶다.
		FActorSpawnParameters params;
		params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto spectator = GetWorld()->SpawnActor<ASpectatorPawn>(GM->SpectatorClass , player->GetActorTransform() , params);
		Possess(spectator);

		// 이전 플레이어를 제거하고싶다.
		player->Destroy();

		// 5초 후에 플레이어를 리스폰 하고 싶다.
		FTimerHandle handle;
		GetWorldTimerManager().SetTimer(handle , this , &ANetPlayerController::ServerRPCRespawnPlayer_Implementation , 5 , false);
	}
}
