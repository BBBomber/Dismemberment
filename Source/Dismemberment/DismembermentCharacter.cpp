// Copyright Epic Games, Inc. All Rights Reserved.

#include "DismembermentCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


//////////////////////////////////////////////////////////////////////////
// ADismembermentCharacter

ADismembermentCharacter::ADismembermentCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	DismembermentComp = CreateDefaultSubobject<USkeletalMeshDismembermentComp>(TEXT("DismembermentComp"));

	CurrentWeapon = nullptr;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ADismembermentCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	SpawnAndEquipWeapon();
}

void ADismembermentCharacter::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
	if (DismembermentComp)
	{
		IDismemberable::Execute_ProcessHit(DismembermentComp, HitData);
	}
}

bool ADismembermentCharacter::CanBeDismembered_Implementation() const
{
	if (DismembermentComp)
	{
		return IDismemberable::Execute_CanBeDismembered(DismembermentComp);
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
// Input

void ADismembermentCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADismembermentCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADismembermentCharacter::Look);

		//attacking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ADismembermentCharacter::Attack);

	}

}

void ADismembermentCharacter::Move(const FInputActionValue& Value)
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

void ADismembermentCharacter::Look(const FInputActionValue& Value)
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

void ADismembermentCharacter::Attack(const FInputActionValue& Value)
{
	if (!AttackMontage || !CurrentWeapon)
	{
		return;
	}

	GetCharacterMovement()->DisableMovement();

	float MontageDuration = PlayAnimMontage(AttackMontage);

	GetWorldTimerManager().SetTimer(AttackEndTimer, this, &ADismembermentCharacter::ReEnableMovement, MontageDuration, false);
}

void ADismembermentCharacter::ReEnableMovement()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ADismembermentCharacter::SpawnAndEquipWeapon()
{
	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Error, TEXT("WeaponClass is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Spawning weapon"));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponActor>(WeaponClass, SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("CurrentWeapon is %s"), CurrentWeapon ? TEXT("valid") : TEXT("null"));

	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActor failed"));
		return;
	}

	CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("hand_r")
	);

	CurrentWeapon->SetActorRelativeLocation(WeaponLocationOffset);
	CurrentWeapon->SetActorRelativeRotation(WeaponRotationOffset);
}




