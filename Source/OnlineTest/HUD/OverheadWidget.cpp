// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "DebugStatic.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(const FString& Text) {
	if (DisplayText) {
		DisplayText->SetText(FText::FromString(Text));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn) {
	ENetRole NetRole = InPawn->GetLocalRole();
	FString RoleString;
	switch (NetRole) {
	case ROLE_None:
		RoleString = "None";
		break;
	case ROLE_SimulatedProxy:
		RoleString = "SimulatedProxy";
		break;
	case ROLE_AutonomousProxy:
		RoleString = "AutonomousProxy";
		break;
	case ROLE_Authority:
		RoleString = "Authority";
		break;
	default:
		RoleString = "Unknown";
		break;
	}

	FString LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *RoleString);
	SetDisplayText(LocalRoleString);
}

void UOverheadWidget::NativeConstruct() {
	RemoveFromParent();
	Super::NativeConstruct();
}
