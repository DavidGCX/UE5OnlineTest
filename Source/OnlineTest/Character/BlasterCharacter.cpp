// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "DebugStatic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "OnlineTest/BlasterComponent/CombatComponent.h"
#include "OnlineTest/Weapon/Weapon.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter() {
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	// DOREPLIFETIME(ABlasterCharacter, OverlappingWeapon);
}

void ABlasterCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();
	if (Combat) {
		Combat->OwnerCharacter = this;
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay() {
	Super::BeginPlay();
	if (APlayerController* PC = Cast<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())) {
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	if (auto playerState = GetPlayerState()) {
		DebugStatic::DebugPrintScreen(FString::Printf(TEXT("Player name: %s"), *playerState->GetPlayerName()),
		                              FColor::Yellow);
	}
}


// Called every frame
void ABlasterCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Equip);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this,
		                                   &ABlasterCharacter::CrouchPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ABlasterCharacter::AimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ABlasterCharacter::StopAiming);
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to get EnhancedInputComponent"));
	}
}


void ABlasterCharacter::Move(const FInputActionValue& Value) {
	FVector2D MoveVector = Value.Get<FVector2d>();
	if (Controller && MoveVector.SizeSquared() > 0.0f) {
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MoveVector.Y);
		AddMovementInput(RightDirection, MoveVector.X);
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value) {
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr) {
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABlasterCharacter::Equip(const FInputActionValue& Value) {
	DebugStatic::DebugPrintScreen("Equip", FColor::Green);
	if (Combat) {
		if (HasAuthority()) {
			Combat->EquipWeapon(OverlappingWeapon);
		} else {
			ServerEquipWeapon();
		}
	}
}

void ABlasterCharacter::CrouchPressed(const FInputActionValue& Value) {
	DebugStatic::DebugPrintScreen("Crouch", FColor::Green);
	if (bIsCrouched) {
		UnCrouch();
	} else {
		Crouch();
	}
}

void ABlasterCharacter::AimPressed(const FInputActionValue& Value) {
	if (Combat) {
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::StopAiming() {
	if (Combat) {
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::ServerEquipWeapon_Implementation() {
	if (Combat) {
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon) {
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(false);
	}
	// This will trigger OnRep_OverlappingWeapon on the client
	OverlappingWeapon = Weapon;
	// Only calls on the server
	if (IsLocallyControlled()) {
		if (OverlappingWeapon) {
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const {
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const {
	return Combat && Combat->bAiming;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* OldWeapon) {
	// Only calls on the client
	// When the new values is null we proceed to the next block with OldWeapon defined.
	if (OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (OldWeapon) {
		OldWeapon->ShowPickupWidget(false);
	}
}
