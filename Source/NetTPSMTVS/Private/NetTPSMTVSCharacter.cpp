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

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANetTPSMTVSCharacter

ANetTPSMTVSCharacter::ANetTPSMTVSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

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
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	HandComp = CreateDefaultSubobject<USceneComponent>(TEXT("HandComp"));
	HandComp->SetupAttachment(GetMesh(), TEXT("PistolPosition"));
	HandComp->SetRelativeLocationAndRotation(FVector(-16.506365f, 2.893501f, 4.275412f) , FRotator(15.481338f, 82.613271f, 8.578510f));
}

void ANetTPSMTVSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	// 태어날 때 모든 총 목록을 기억하고싶다.
	FName tag = TEXT("Pistol");
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld() , AActor::StaticClass() , tag , PistolList);
}


//////////////////////////////////////////////////////////////////////////
// Input

void ANetTPSMTVSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANetTPSMTVSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANetTPSMTVSCharacter::Look);

		EnhancedInputComponent->BindAction(GrabPistolAction, ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::GrabPistol);
		
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started , this , &ANetTPSMTVSCharacter::FirePistol);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANetTPSMTVSCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANetTPSMTVSCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
AActor* tempOwner;
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

		// 그 총을 기억하고싶다. (GrabPistolActor)
		GrabPistolActor = pistol;
		// 잡은총의 소유자를 나로 하고싶다. -> 액터의 오너는 플레이어 컨트롤러이다.
		pistol->SetOwner(this);
		bHasPistol = true;

		tempOwner = pistol->GetOwner();

		// 총액터를 HandComp에 붙이고싶다.
		AttachPistol(GrabPistolActor);
		break;
	}
}

void ANetTPSMTVSCharacter::MyReleasePistol()
{
	// 총을 이미 잡은 상태 -> 놓고싶다.
	if ( bHasPistol )
	{
		bHasPistol = false;
	}

	// 총의 오너를 취소하고싶다.
	if ( GrabPistolActor )
	{
		DetachPistol();

		GrabPistolActor->SetOwner(nullptr);
		// 총을 잊고싶다.
		GrabPistolActor = nullptr;
	}
}

void ANetTPSMTVSCharacter::AttachPistol(AActor* pistolActor)
{
	GrabPistolActor = pistolActor;
	auto* mesh = GrabPistolActor->GetComponentByClass<UStaticMeshComponent>();
	check(mesh);
	if ( mesh )
	{
		mesh->SetSimulatePhysics(false);
		mesh->AttachToComponent(HandComp , FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void ANetTPSMTVSCharacter::DetachPistol()
{
	// 총의 메쉬를 가져와서
	auto* mesh = GrabPistolActor->GetComponentByClass<UStaticMeshComponent>();
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
	if ( false == bHasPistol )
		return;

	// Fire몽타주를 재생하고싶다.
	auto* anim = CastChecked<UNetTpsPlayerAnim>(GetMesh()->GetAnimInstance());
	if ( anim ){
		anim->PlayFireMontage();
	}


	// 카메라 위치에서 카메라 앞 방향으로 1Km 선을 쏘고싶다.
	FVector start = FollowCamera->GetComponentLocation();
	FVector end = start + FollowCamera->GetForwardVector() * 100000.f;
	FHitResult hitInfo;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo , start , end , ECC_Visibility , params);
	// 만약 부딪힌것이 있다면
	if (bHit)
	{
		// 그곳에 BulletImpactVFX를 생성해서 배치하고싶다.
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld() , BulletImpactVFX , hitInfo.ImpactPoint);
	}


}


