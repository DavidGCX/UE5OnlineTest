// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"


// Custom Delegates for the menu class to bind callbacks to

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);

DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete,
                                     const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionsComplete, EOnJoinSessionCompleteResult::Type Result,
                                     FString SessionName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem {
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();

	// Handl Session Functionality for the menu class to call
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SearchResult);
	void DestroySession();
	void StartSession();

	FMultiplayerOnCreateSessionComplete CreateSessionComplete;
	FMultiplayerOnFindSessionsComplete FindSessionsComplete;
	FMultiplayerOnJoinSessionsComplete JoinSessionComplete;
	FMultiplayerOnDestroySessionComplete DestroySessionComplete;
	FMultiplayerOnStartSessionComplete StartSessionComplete;

protected:
	void DebugPrintScreen(FString Message, FColor Color);
	// Internal Callbacks for Delegates
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	// To Add to the Online Session Interface Delagates List
	// Bind our MultiplayerSessionSubsystem internal callbacks to these;
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate OnStartSessionCompleteDelegate;
	FDelegateHandle OnStartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy{false};
	int LastNumberOfPublicConnections{4};
	FString LastMatchType{"FreeForAll"};
};
