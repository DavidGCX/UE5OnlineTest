// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "DebugStatic.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	OnCreateSessionCompleteDelegate(
		FOnCreateSessionCompleteDelegate::CreateUObject(
			this, &UMultiplayerSessionsSubsystem::OnCreateSessionComplete)),
	OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::OnFindSessionsComplete)),
	OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::OnJoinSessionComplete)),
	OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::OnDestroySessionComplete)),
	OnStartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::OnStartSessionComplete)) {
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) {
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to get online subsystem"));
	}
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType) {
	if (!SessionInterface.IsValid()) {
		UE_LOG(LogTemp, Error, TEXT("SessionInterface is not valid"));
		return;
	}
	if (auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession)) {
		bCreateSessionOnDestroy = true;
		LastNumberOfPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		DestroySession();
		return;
	}
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		OnCreateSessionCompleteDelegate);
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	LastSessionSettings->Set(FName("MatchType"), MatchType,
	                         EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;
	ULocalPlayer* Player = GetWorld()->GetFirstLocalPlayerFromController();
	if (Player) {
		if (!SessionInterface->CreateSession(*Player->GetPreferredUniqueNetId(), NAME_GameSession,
		                                     *LastSessionSettings)) {
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
			// Broadcast our own custom Delegate, (Not Sure why not broadcast in the OnCreateSessionComplete Directly)
			CreateSessionComplete.Broadcast(false);
		}
	} else {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		CreateSessionComplete.Broadcast(false);
		UE_LOG(LogTemp, Error, TEXT("Failed to get player"));
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults) {
	if (!SessionInterface.IsValid()) {
		UE_LOG(LogTemp, Error, TEXT("SessionInterface is not valid"));
		return;
	}
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		OnFindSessionsCompleteDelegate);
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL";
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef())) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		FindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SearchResult) {
	if (!SessionInterface.IsValid()) {
		JoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError, "");
		UE_LOG(LogTemp, Error, TEXT("SessionInterface is not valid"));
		return;
	}

	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle
		(OnJoinSessionCompleteDelegate);
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult)) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		JoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError, "");
	}
}

void UMultiplayerSessionsSubsystem::DestroySession() {
	if (!SessionInterface.IsValid()) {
		DestroySessionComplete.Broadcast(false);
		UE_LOG(LogTemp, Error, TEXT("SessionInterface is not valid"));
		return;
	}
	OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		OnDestroySessionCompleteDelegate);
	if (!SessionInterface->DestroySession(NAME_GameSession)) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		DestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession() {}

void UMultiplayerSessionsSubsystem::DebugPrintScreen(FString Message, FColor Color) {
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
		                                 Message);
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to get GEngine"));
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	CreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	}
	if (LastSessionSearch->SearchResults.Num() <= 0) {
		FindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	FindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::
OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	if (SessionInterface && SessionInterface.IsValid()) {
		FString Address;
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

		SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
		JoinSessionComplete.Broadcast(Result, Address);
	} else {
		JoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError, "");
	}
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful) {
	if (SessionInterface) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
	}
	if (bWasSuccessful && bCreateSessionOnDestroy) {
		CreateSession(LastNumberOfPublicConnections, LastMatchType);
		bCreateSessionOnDestroy = false;
	}
	DestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful) {}
