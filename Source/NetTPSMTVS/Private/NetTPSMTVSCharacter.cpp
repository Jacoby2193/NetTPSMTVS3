// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetTPSMTVSCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "NetTpsPlayerAnim.h"
#include "MainWidget.h"
#include "Components/WidgetComponent.h"
#include "HealthBar.h"
#include "GameFramework/PlayerController.h"
#include "NetTPSMTVS.h"
#include "Net/UnrealNetwork.h"
#include "Components/HorizontalBox.h"
#include "NetPlayerController.h"
#include "NetTPSPlayerState.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANetTPSMTVSCharacter

ANetTPSMTVSCharacter::ANetTPSMTVSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f , 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f , 500.0f , 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 150.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SetRelativeLocation(FVector(0 , 40 , 60));


	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom , USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	HandComp = CreateDefaultSubobject<USceneComponent>(TEXT("HandComp"));
	HandComp->SetupAttachment(GetMesh() , TEXT("PistolPosition"));
	HandComp->SetRelativeLocationAndRotation(FVector(-16.506365f , 2.893501f , 4.275412f) , FRotator(15.481338f , 82.613271f , 8.578510f));

	HPUIComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HPUIComp->SetupAttachment(GetMesh());

	// 네트워크 동기화옵션 활성화
	bReplicates = true;
	SetReplicateMovement(true);
}

void ANetTPSMTVSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// 태어날 때 모든 총 목록을 기억하고싶다.
	FName tag = TEXT("Pistol");
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld() , AActor::StaticClass() , tag , PistolList);

	if ( IsLocallyControlled() && false == HasAuthority() )
	{
		InitMainUI();
	}
}

void ANetTPSMTVSCharacter::PossessedBy(AController* NewController)
{
	PRINTLOG(TEXT("Begin"));
	Super::PossessedBy(NewController);

	if ( IsLocallyControlled() )
	{
		InitMainUI();
	}

	PRINTLOG(TEXT("End"));
}

void ANetTPSMTVSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if ( HPUIComp )
	{
		//FVector camLoc = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
		FVector camLoc = UGameplayStatics::GetPlayerCameraManager(GetWorld() , 0)->GetCameraLocation();
		FVector direction = camLoc - HPUIComp->GetComponentLocation();
		direction.Z = 0.f;

		HPUIComp->SetWorldRotation(direction.GetSafeNormal().ToOrientationRotator());
	}

	PrintNetLog();
}


//////////////////////////////////////////////////////////////////////////
// Input

void ANetTPSMTVSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if ( APlayerController* PlayerController = Cast<APlayerController>(GetController()) )
	{
		if ( UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) )
		{
			Subsystem->AddMappingContext(DefaultMappingContext , 0);
		}
	}

	// Set up action bindings
	if ( UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent) ) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction , ETriggerEvent::Started , this , &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction , ETriggerEvent::Completed , this , &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction , ETriggerEvent::Triggered , this , &ANetTPSMTVSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction , ETriggerEvent::Triggered , this , &ANetTPSMTVSCharacter::Look);

		EnhancedInputComponent->BindAction(GrabPistolAction , ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::GrabPistol);

		EnhancedInputComponent->BindAction(FireAction , ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::FirePistol);

		EnhancedInputComponent->BindAction(ReloadAction , ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::ReloadPistol);
		
		EnhancedInputComponent->BindAction(VoiceChatAction, ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::StartVoiceChat);

		EnhancedInputComponent->BindAction(VoiceChatAction, ETriggerEvent::Completed
			, this , &ANetTPSMTVSCharacter::CancleVoiceChat);

	}
	else
	{
		UE_LOG(LogTemplateCharacter , Error , TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file.") , *GetNameSafe(this));
	}
}

void ANetTPSMTVSCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if ( Controller != nullptr )
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0 , Rotation.Yaw , 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection , MovementVector.Y);
		AddMovementInput(RightDirection , MovementVector.X);
	}
}

void ANetTPSMTVSCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if ( Controller != nullptr )
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ANetTPSMTVSCharacter::GrabPistol(const FInputActionValue& Value)
{
	if ( bHasPistol )
	{
		MyReleasePistol();
	}
	else
	{
		MyTakePistol();
	}
}

void ANetTPSMTVSCharacter::MyTakePistol()
{
	ServerRPCTakePistol();
}

void ANetTPSMTVSCharacter::MyReleasePistol()
{
	// 총을 잡고 있지 않거나 재장전 중이면 총을 버릴 수 없다.
	if ( false == bHasPistol || IsReloading || false == IsLocallyControlled() )
		return;

	ServerRPCReleasePistol();
}

void ANetTPSMTVSCharacter::AttachPistol(AActor* pistolActor)
{
	if ( IsLocallyControlled() && MainUI )
		MainUI->SetActivePistolUI(true);

	GrabPistolActor = pistolActor;
	auto* mesh = GrabPistolActor->GetComponentByClass<UStaticMeshComponent>();
	check(mesh);
	if ( mesh )
	{
		mesh->SetSimulatePhysics(false);
		mesh->AttachToComponent(HandComp , FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void ANetTPSMTVSCharacter::DetachPistol(AActor* pistolActor)
{
	if ( IsLocallyControlled() && MainUI )
	{
		MainUI->SetActivePistolUI(false);
	}

	// 총의 메쉬를 가져와서
	auto* mesh = pistolActor->GetComponentByClass<UStaticMeshComponent>();
	check(mesh);
	if ( mesh )
	{
		// 물리를 켜주고싶다.
		mesh->SetSimulatePhysics(true);
		// 분리하고싶다..
		mesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}
}

void ANetTPSMTVSCharacter::FirePistol(const FInputActionValue& Value)
{
	if ( false == bHasPistol || true == IsReloading || nullptr == GrabPistolActor )
		return;

	// 만약 총알이 없다면 바로 종료
	if ( BulletCount <= 0 )
		return;

	ServerRPCFire();

}

void ANetTPSMTVSCharacter::ReloadPistol(const FInputActionValue& Value)
{
	// 총 소지중이 아니거나 재장전 중이라면 아무 처리 하지 않는다.
	if ( !bHasPistol || IsReloading )
	{
		return;
	}

	auto* anim = Cast<UNetTpsPlayerAnim>(GetMesh()->GetAnimInstance());
	if ( anim )
	{
		IsReloading = true;
		anim->PlayReloadMontage();
	}
}

void ANetTPSMTVSCharacter::StartVoiceChat(const FInputActionValue& Value)
{
	GetController<ANetPlayerController>()->StartTalking();
}

void ANetTPSMTVSCharacter::CancleVoiceChat(const FInputActionValue& Value)
{
	GetController<ANetPlayerController>()->StopTalking();
}

void ANetTPSMTVSCharacter::InitMainUI()
{
	PRINTLOG(TEXT("[%s] Begin") , Controller ? TEXT("Player") : TEXT("Not Player"));

	// Player가 제어중이 아니라면 처리하지 않는다.
	auto* pc = Cast<ANetPlayerController>(Controller);
	if ( nullptr == pc )
	{
		MainUI = nullptr;
		return;
	}

	if ( pc->MainUIWidget )
	{
		if ( nullptr == pc->MainUI )
		{
			pc->MainUI = CastChecked<UMainWidget>(CreateWidget(GetWorld() , pc->MainUIWidget));
		}
		MainUI = pc->MainUI;
		MainUI->AddToViewport();
		MainUI->SetActivePistolUI(false);
		// 남은 총알을 다 삭제하고 다시 최대치로 생성해준다.
		MainUI->RemoveAllBulletUI();
		MainUI->InitBulletUI(MaxBulletCount);

		hp = MaxHP;
		MainUI->HP = 1.0f;

		// MainUI가 있기 때문에 머리위의 HPUIComp는 비활성화 하고싶다.
		if ( HPUIComp )
		{
			HPUIComp->SetVisibility(false);
		}
	}

}

void ANetTPSMTVSCharacter::InitBulletUI()
{
	ServerRPCReload();
}

void ANetTPSMTVSCharacter::OnRep_HP()
{
	// 죽음처리
	if ( HP <= 0 )
	{
		IsDead = true;
		if ( bHasPistol )
		{
			GrabPistol(FInputActionValue());
		}
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->DisableMovement();
	}

	// UI에 할당할 퍼센트 계산
	float percent = hp / MaxHP;

	if ( MainUI )
	{
		MainUI->HP = percent;
		// 피격효과 처리
		MainUI->PlayDamageAnimation();
		if ( DamageCameraShake )
		{
			auto* pc = Cast<APlayerController>(Controller);
			if ( pc )
			{
				pc->ClientStartCameraShake(DamageCameraShake);
			}
		}
	}
	else
	{
		// MainUI가 없을때는 다른 플레이어다. HPUIComp에 적용해주자
		auto* hpUI = Cast<UHealthBar>(HPUIComp->GetWidget());
		if ( hpUI )
			hpUI->HP = percent;
	}
}

float ANetTPSMTVSCharacter::GetHP()
{
	return hp;
}

void ANetTPSMTVSCharacter::SetHP(float value)
{
	hp = value;
	OnRep_HP();
}

void ANetTPSMTVSCharacter::DamageProcess()
{
	// 체력을 감소시킨다.
	HP--;

	// 죽음처리
	if ( HP <= 0 )
	{
		IsDead = true;
	}
}

void ANetTPSMTVSCharacter::DieProcess()
{
	auto* pc = Cast<APlayerController>(Controller);
	if ( pc )
	{
		pc->SetShowMouseCursor(true);
	}
	GetFollowCamera()->PostProcessSettings.ColorSaturation = FVector4(0 , 0 , 0 , 1);

	// Die UI표시
	if ( IsLocallyControlled() && MainUI )
	{
		MainUI->GameoverUI->SetVisibility(ESlateVisibility::Visible);
	}

}

void ANetTPSMTVSCharacter::PrintNetLog()
{
	const FString conStr = GetNetConnection() ? TEXT("Valid Connection") : TEXT("Invalid Connection");
	const FString ownerName = GetOwner() ? GetOwner()->GetName() : TEXT("No Owner");

	FString logStr = FString::Printf(TEXT("Connection : %s\nOwner Name : %s\nLocal Role : %s\nRemote Role : %s") , *conStr , *ownerName , *LOCALROLE , *REMOTEROLE);
	FVector loc = GetActorLocation() + GetActorUpVector() * 30;
	DrawDebugString(GetWorld() , loc , logStr , nullptr , FColor::Yellow , 0 , true , 1.f);
}

void ANetTPSMTVSCharacter::ServerRPCTakePistol_Implementation()
{
	// 총을 잡지 않은 상태 -> 잡고싶다.
	// 총목록을 검사하고싶다.
	for ( AActor* pistol : PistolList )
	{
		// 나와 총과의 거리가 GrabDistance 이하라면
		// 그 중에 소유자가 없는 총이라면
		float tempDist = GetDistanceTo(pistol);
		if ( tempDist > GrabDistance )
			continue;
		if ( nullptr != pistol->GetOwner() )
			continue;

		// 잡은총의 소유자를 나로 하고싶다. -> 액터의 오너는 플레이어 컨트롤러이다.
		pistol->SetOwner(this);

		// 그 총을 기억하고싶다. (GrabPistolActor)
		GrabPistolActor = pistol;

		bHasPistol = true;

		MulticastRPCTakePistol(pistol);
		break;
	}
}

void ANetTPSMTVSCharacter::MulticastRPCTakePistol_Implementation(AActor* pistolActor)
{
	// 총액터를 HandComp에 붙이고싶다.
	AttachPistol(pistolActor);
}

void ANetTPSMTVSCharacter::ServerRPCReleasePistol_Implementation()
{
	// 총을 이미 잡은 상태 -> 놓고싶다.
	if ( bHasPistol )
	{
		bHasPistol = false;
	}

	// 총의 오너를 취소하고싶다.
	if ( GrabPistolActor )
	{
		MulticastRPCReleasePistol(GrabPistolActor);

		GrabPistolActor->SetOwner(nullptr);
		// 총을 잊고싶다.
		GrabPistolActor = nullptr;
	}
}

void ANetTPSMTVSCharacter::MulticastRPCReleasePistol_Implementation(AActor* pistolActor)
{
	if ( pistolActor )
	{
		// 총 분리
		DetachPistol(pistolActor);
	}
}

void ANetTPSMTVSCharacter::ServerRPCFire_Implementation()
{
	// 총알 1발 감소하고 UI에 반영하고 싶다.
	BulletCount--;

	// 카메라 위치에서 카메라 앞 방향으로 1Km 선을 쏘고싶다.
	FVector start = FollowCamera->GetComponentLocation();
	FVector end = start + FollowCamera->GetForwardVector() * 100000.f;
	FHitResult hitInfo;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo , start , end , ECC_Visibility , params);

	// 만약 부딪힌것이 있다면
	if ( bHit )
	{
		// 맞은 대상이 상대방일 경우 데미지 처리
		auto* otherPlayer = Cast<ANetTPSMTVSCharacter>(hitInfo.GetActor());
		if ( otherPlayer )
		{
			otherPlayer->DamageProcess();
			// 점수를 1점 증가 시키고 싶다.
			auto* ps = Cast<ANetTPSPlayerState>(GetPlayerState());
			ps->SetScore(ps->GetScore() + 1);
		}
	}

	MulticastRPCFire(bHit , hitInfo);
}

void ANetTPSMTVSCharacter::MulticastRPCFire_Implementation(bool bHit , const FHitResult hitInfo)
{
	if ( IsLocallyControlled() && MainUI )
		MainUI->RemoveBulletUI();

	// Fire몽타주를 재생하고싶다.
	auto* anim = CastChecked<UNetTpsPlayerAnim>(GetMesh()->GetAnimInstance());
	if ( anim )
	{
		anim->PlayFireMontage();
	}

	// 만약 부딪힌것이 있다면
	if ( bHit )
	{
		// 그곳에 BulletImpactVFX를 생성해서 배치하고싶다.
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld() , BulletImpactVFX , hitInfo.ImpactPoint);
	}
}

void ANetTPSMTVSCharacter::ServerRPCReload_Implementation()
{
	BulletCount = MaxBulletCount;
	ClientRPCReload();
}

void ANetTPSMTVSCharacter::ClientRPCReload_Implementation()
{
	if ( MainUI )
	{
		MainUI->RemoveAllBulletUI();

		//MainUI->InitBulletUI(MaxBulletCount);
		for ( int32 i = 0; i < MaxBulletCount; i++ )
		{
			MainUI->AddBulletUI();
		}
	}

	// 재장전 완료상태 처리
	IsReloading = false;
}

void ANetTPSMTVSCharacter::ServerRPCChat_Implementation(const FString& msg)
{
	MultiRPCChat(msg);
}

void ANetTPSMTVSCharacter::MultiRPCChat_Implementation(const FString& msg)
{
	// UI의 AddChatMessage를 호출하고싶다.
	auto* pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
	if ( pc->MainUI )
	{
		pc->MainUI->AddChatMessage(msg);
	}
}

void ANetTPSMTVSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetTPSMTVSCharacter , bHasPistol);
	DOREPLIFETIME(ANetTPSMTVSCharacter , BulletCount);
	DOREPLIFETIME(ANetTPSMTVSCharacter , hp);
}

