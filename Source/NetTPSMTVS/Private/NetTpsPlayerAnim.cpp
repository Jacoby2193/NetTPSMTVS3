// Fill out your copyright notice in the Description page of Project Settings.


#include "NetTpsPlayerAnim.h"
#include "NetTPSMTVSCharacter.h"

void UNetTpsPlayerAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Me = Cast<ANetTPSMTVSCharacter>(TryGetPawnOwner());
}

void UNetTpsPlayerAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if ( nullptr == Me )
		return;
	// 아래 두 값을 채우시오.
	FVector vel = Me->GetVelocity();
	FVector right = Me->GetActorRightVector();
	FVector forward = Me->GetActorForwardVector();

	Horizontal = FVector::DotProduct(vel , right);	// 좌우
	
	Vertical = FVector::DotProduct(vel , forward);	// 앞뒤

	bHasPistol = Me->bHasPistol;

	PitchAngle = -Me->GetBaseAimRotation().Pitch;
}

void UNetTpsPlayerAnim::PlayFireMontage()
{
	if (bHasPistol && FireMontage)
	{
		Montage_Play(FireMontage);
	}
}
