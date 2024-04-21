// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "DebugStatic.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void ALobbyGameMode::OnPostLogin(AController* NewPlayer) {
	Super::OnPostLogin(NewPlayer);
	if (GameState) {
		int NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		DebugStatic::DebugPrintScreen(FString::Printf(TEXT("Number of players: %d"), NumberOfPlayers), FColor::Yellow);
		if (APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>()) {
			DebugStatic::DebugPrintScreen(FString::Printf(TEXT("Player name: %s"), *PlayerState->GetPlayerName()),
			                              FColor::Yellow);
		}
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer) {
	Super::PostLogin(NewPlayer);
	int NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberOfPlayers == 2) {
		UWorld* World = GetWorld();
		if (World) {
			bUseSeamlessTravel = true;
			World->ServerTravel("/Game/Maps/BlasterMap?listen");
		}
	}
}

void ALobbyGameMode::Logout(AController* Exiting) {
	Super::Logout(Exiting);
	if (GameState) {
		int NumberOfPlayers = GameState.Get()->PlayerArray.Num();
		DebugStatic::DebugPrintScreen(FString::Printf(TEXT("Number of players: %d"), NumberOfPlayers - 1),
		                              FColor::Yellow);
		if (APlayerState* PlayerState = Exiting->GetPlayerState<APlayerState>()) {
			DebugStatic::DebugPrintScreen(
				FString::Printf(TEXT("Player name: %s Exit the Game"), *PlayerState->GetPlayerName()),
				FColor::Yellow);
		}
	}
}
