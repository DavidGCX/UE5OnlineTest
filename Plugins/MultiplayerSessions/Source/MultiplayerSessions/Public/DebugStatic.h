// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class MULTIPLAYERSESSIONS_API DebugStatic {
public:
	DebugStatic();
	~DebugStatic();
	static void DebugPrintScreen(FString Message, FColor Color = FColor::Green);
};
