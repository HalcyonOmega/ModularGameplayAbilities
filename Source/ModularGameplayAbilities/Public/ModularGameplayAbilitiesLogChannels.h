// Copyright Chronicler.

#pragma once

#include "CoreMinimal.h"

MODULARGAMEPLAYABILITIES_API DECLARE_LOG_CATEGORY_EXTERN(LogModularGameplayAbilities, Log, All);

#define MGA_LOG(Verbosity, Format, ...) \
{ \
UE_LOG(LogModularGameplayAbilities, Verbosity, Format, ##__VA_ARGS__); \
}

#define MGA_NS_LOG(Verbosity, Format, ...) \
{ \
UE_LOG(LogModularGameplayAbilities, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}