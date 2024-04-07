// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetUp(int32 NumPublicConnections = 4, FString MatchType = "FreeForAll");

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;
	// Callbacks for custom delegates on multiplayer session subsystem
	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);

	void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);

	void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result, FString Address);
	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSessionComplete(bool bWasSuccessful);

private:
	UPROPERTY(meta=(BindWidget))
	class UButton* HostButton;
	UPROPERTY(meta=(BindWidget))
	class UButton* JoinButton;
	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void JoinButtonClicked();
	void MenuTearDown();
	// The subsystem that will handle the session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{"FreeForAll"};
};
