// Fill out your copyright notice in the Description page of Project Settings.


#include "NetActor.h"
#include "NetTPSMTVS.h"
#include "EngineUtils.h"
#include "NetTPSMTVSCharacter.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ANetActor::ANetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	// Scale도 절반으로 하고 싶다.
	MeshComp->SetRelativeScale3D(FVector(0.5f));

	bReplicates = true;

}

// Called when the game starts or when spawned
void ANetActor::BeginPlay()
{
	Super::BeginPlay();

	Mat = MeshComp->CreateDynamicMaterialInstance(0);

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(handle , [&](){
			//MatColor = FLinearColor(FMath::RandRange(0.0f, 0.3f), FMath::RandRange(0.0f , 0.3f), FMath::RandRange(0.0f , 0.3f), 1.f);
			
			MatColor = FLinearColor::MakeRandomColor();
			OnRep_ChangeMatColor();

		} , 1 , true);
	}
}

void ANetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorldTimerManager().ClearTimer(handle);
}

// Called every frame
void ANetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PrintNetLog();

	FindOwner();

	if ( HasAuthority() )
	{
		AddActorLocalRotation(FRotator(0 , 50 * DeltaTime , 0));
		RotYaw = GetActorRotation().Yaw;
	}
	//else
	//{
	//	FRotator newRot = GetActorRotation();
	//	newRot.Yaw = RotYaw;
	//	SetActorRotation(newRot);
	//}
}

void ANetActor::PrintNetLog()
{
	const FString conStr = GetNetConnection() ? TEXT("Valid Connection") : TEXT("Invalid Connection");
	const FString ownerName = GetOwner() ? GetOwner()->GetName() : TEXT("No Owner");

	FString logStr = FString::Printf(TEXT("Connection : %s\nOwner Name : %s\nLocal Role : %s\nRemote Role : %s") , *conStr , *ownerName , *LOCALROLE , *REMOTEROLE);
	FVector loc = GetActorLocation() + GetActorUpVector() * 30;
	DrawDebugString(GetWorld() , loc , logStr , nullptr , FColor::Yellow , 0 , true , 1.f);
}

void ANetActor::FindOwner()
{
	if ( HasAuthority() )
	{
		AActor* newOwner = nullptr;
		float minDist = SearchDistance;

		for ( TActorIterator<ANetTPSMTVSCharacter> it(GetWorld()); it; ++it )
		{
			AActor* otherActor = *it;
			float dist = GetDistanceTo(otherActor);

			if ( dist < SearchDistance )
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

	DrawDebugSphere(GetWorld() , GetActorLocation() , SearchDistance , 16 , FColor::Cyan , false , 0 , 0 , 1);
}

void ANetActor::OnRep_RotYaw()
{
	FRotator newRot = GetActorRotation();
	newRot.Yaw = RotYaw;
	SetActorRotation(newRot);
}

void ANetActor::OnRep_ChangeMatColor()
{
	if (Mat)
	{
		Mat->SetVectorParameterValue(TEXT("FloorColor") , MatColor);
	}
}

void ANetActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetActor , RotYaw);
	DOREPLIFETIME(ANetActor , MatColor);
}

