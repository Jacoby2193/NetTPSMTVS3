// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NetTpsPlayerAnim.generated.h"

/**
 * 
 */
UCLASS()
class NETTPSMTVS_API UNetTpsPlayerAnim : public UAnimInstance
{
	GENERATED_BODY()


public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class ANetTPSMTVSCharacter* Me;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bHasPistol;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Horizontal;	// Direction

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Vertical;		// Speed

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* FireMontage;

	void PlayFireMontage();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float PitchAngle;



	
};
