// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "DebugStatic.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"

void UMenu::MenuSetUp(int32 NumConnections, FString TypeMatch) {
	NumPublicConnections = NumConnections;
	MatchType = TypeMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	if (UGameInstance const* GameInstance = GetGameInstance()) {
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->CreateSessionComplete.AddDynamic(this, &UMenu::OnCreateSessionComplete);
		MultiplayerSessionsSubsystem->FindSessionsComplete.AddUObject(this, &UMenu::OnFindSessionsComplete);
		MultiplayerSessionsSubsystem->JoinSessionComplete.AddUObject(this, &UMenu::OnJoinSessionComplete);
		MultiplayerSessionsSubsystem->DestroySessionComplete.AddDynamic(this, &UMenu::OnDestroySessionComplete);
		MultiplayerSessionsSubsystem->StartSessionComplete.AddDynamic(this, &UMenu::OnStartSessionComplete);
	}
}

bool UMenu::Initialize() {
	bool Success = Super::Initialize();
	if (!Success) return false;
	if (!HostButton) return false;
	if (!JoinButton) return false;
	HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	return true;
}

void UMenu::NativeDestruct() {
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSessionComplete(bool bWasSuccessful) {
	if (bWasSuccessful) {
		UWorld* World = GetWorld();
		if (World) {
			World->ServerTravel("/Game/ThirdPerson/Maps/Lobby?listen");
		}
		DebugStatic::DebugPrintScreen("Session Created", FColor::Green);
	} else {
		DebugStatic::DebugPrintScreen("Failed to create session", FColor::Red);
	}
}

void UMenu::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful) {
	if (!bWasSuccessful || MultiplayerSessionsSubsystem == nullptr || SearchResults.Num() == 0) {
		DebugStatic::DebugPrintScreen("Failed to find sessions", FColor::Red);
		return;
	}

	for (auto Result : SearchResults) {
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		if (SettingsValue == MatchType) {
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
}

void UMenu::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result, FString Address) {
	if (Result == EOnJoinSessionCompleteResult::Success) {
		APlayerController* playerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (playerController != nullptr) {
			playerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		}
	} else {
		DebugStatic::DebugPrintScreen("Failed to join session", FColor::Red);
	}
}

void UMenu::OnDestroySessionComplete(bool bWasSuccessful) {}
void UMenu::OnStartSessionComplete(bool bWasSuccessful) {}

void UMenu::HostButtonClicked() {
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked() {
	if (MultiplayerSessionsSubsystem) {
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown() {
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController) {
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
