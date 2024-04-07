// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugStatic.h"

DebugStatic::DebugStatic() {}

DebugStatic::~DebugStatic() {}

void DebugStatic::DebugPrintScreen(FString Message, FColor Color) {
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
		                                 Message);
	} else {
		UE_LOG(LogTemp, Error, TEXT("Failed to get GEngine"));
	}
}
