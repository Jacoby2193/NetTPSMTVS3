// Fill out your copyright notice in the Description page of Project Settings.


#include "NetActor.h"
#include "NetTPSMTVS.h"
#include "EngineUtils.h"
#include "NetTPSMTVSCharacter.h"

// Sets default values
ANetActor::ANetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	// Scale도 절반으로 하고 싶다.
	MeshComp->SetRelativeScale3D(FVector(0.5f));

}

// Called when the game starts or when spawned
void ANetActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ANetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PrintNetLog();

	if ( HasAuthority() )
	{
		AActor* newOwner = nullptr;
		float minDist = searchDistance;

		for ( TActorIterator<ANetTPSMTVSCharacter> it(GetWorld()); it; ++it )
		{
			AActor* otherActor = *it;
			float dist = GetDistanceTo(otherActor);

			if ( dist < searchDistance )
			{
				minDist = dist;
				newOwner = otherActor;
			}
		}
		// Owner 설정
		if ( newOwner != GetOwner() )
		{
			SetOwner(newOwner);
		}
	}

	DrawDebugSphere(GetWorld() , GetActorLocation() , searchDistance , 16 , FColor::Cyan , false , 0, 0, 1);
}

void ANetActor::PrintNetLog()
{
	const FString conStr = GetNetConnection() ? TEXT("Valid Connection") : TEXT("Invalid Connection");
	const FString ownerName = GetOwner() ? GetOwner()->GetName() : TEXT("No Owner");

	FString logStr = FString::Printf(TEXT("Connection : %s\nOwner Name : %s\nLocal Role : %s\nRemote Role : %s") , *conStr , *ownerName , *LOCALROLE , *REMOTEROLE);
	FVector loc = GetActorLocation() + GetActorUpVector() * 30;
	DrawDebugString(GetWorld() , loc , logStr , nullptr , FColor::Yellow , 0 , true , 1.f);
}

