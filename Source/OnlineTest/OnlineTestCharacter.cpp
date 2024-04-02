// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineTestCharacter.h"
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
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AOnlineTestCharacter

AOnlineTestCharacter::AOnlineTestCharacter(): CreateSessionCompleteDelegate(
	                                              FOnCreateSessionCompleteDelegate::CreateUObject(
		                                              this, &ThisClass::OnCreateSessionComplete)),
                                              FindSessionCompleteDelegate(
	                                              FOnFindSessionsCompleteDelegate::CreateUObject(
		                                              this, &ThisClass::OnFindSessionsComplete)),
                                              JoinSessionCompleteDelegate(
	                                              FOnJoinSessionCompleteDelegate::CreateUObject(
		                                              this, &ThisClass::OnJoinSessionComplete)) {
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
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem != nullptr) {
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
			                                 FString::Printf(TEXT("OnlineSubsystem is valid %s"),
			                                                 *OnlineSubsystem->GetSubsystemName().ToString())
			);
		}
	}
}


void AOnlineTestCharacter::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AOnlineTestCharacter::OpenLobby() {
	UWorld* World = GetWorld();
	if (World != nullptr) {
		FString Address = "/Game/ThirdPerson/Maps/Lobby?listen";
		World->ServerTravel(Address);
	}
}

void AOnlineTestCharacter::CallOpenLevel(const FString& Address) {
	UGameplayStatics::OpenLevel(this, *Address);
}

void AOnlineTestCharacter::CallClientTravel(const FString& Address) {
	APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (playerController != nullptr) {
		playerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}

void AOnlineTestCharacter::CreateGameSession() {
	if (!OnlineSessionInterface.IsValid()) {
		UE_LOG(LogTemplateCharacter, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}
	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr) {
		OnlineSessionInterface->DestroySession(NAME_GameSession);
	}
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = 4;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("MatchType"), FString("FreeForALl"),
	                     EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AOnlineTestCharacter::JoinGameSession() {
	// Find gama session
	if (!OnlineSessionInterface.IsValid()) {
		UE_LOG(LogTemplateCharacter, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}

	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegate);

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 100000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void AOnlineTestCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful) {
	if (bWasSuccessful) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
			                                 FString::Printf(
				                                 TEXT("Successfully created session: %s"), *SessionName.ToString()));
		}
		UWorld* World = GetWorld();
		if (World != nullptr) {
			FString Address = "/Game/ThirdPerson/Maps/Lobby?listen";
			World->ServerTravel(Address);
		}
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
			                                 TEXT("Failed to create session"));
		}
	}
}

void AOnlineTestCharacter::OnFindSessionsComplete(bool bWasSuccessful) {
	if (!OnlineSessionInterface.IsValid()) {
		UE_LOG(LogTemplateCharacter, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}
	for (auto result : SessionSearch->SearchResults) {
		FString Id = result.GetSessionIdStr();
		FString Name = result.Session.OwningUserName;
		FString MatchType;
		result.Session.SessionSettings.Get(FName("MatchType"), MatchType);
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
			                                 FString::Printf(TEXT("Found session ID:%s User:%s"), *Id, *Name));
		}
		if (MatchType == FString("FreeForAll")) {
			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
				                                 FString::Printf(TEXT("Join Match Type: %s"), *MatchType));
			}
			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, result);
		}
	}
}

void AOnlineTestCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	if (!OnlineSessionInterface) {
		UE_LOG(LogTemplateCharacter, Error, TEXT("OnlineSessionInterface is not valid"));
		return;
	}
	FString Address;
	if (OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address)) {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
			                                 FString::Printf(TEXT("Joining session: %s"), *Address));
		}
		CallClientTravel(Address);
	} else {
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red,
			                                 TEXT("Failed to join session"));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AOnlineTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOnlineTestCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AOnlineTestCharacter::Look);
	} else {
		UE_LOG(LogTemplateCharacter, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void AOnlineTestCharacter::Move(const FInputActionValue& Value) {
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr) {
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

void AOnlineTestCharacter::Look(const FInputActionValue& Value) {
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr) {
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
