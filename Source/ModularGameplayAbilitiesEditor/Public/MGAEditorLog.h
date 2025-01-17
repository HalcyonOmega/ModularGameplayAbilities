// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

MODULARGAMEPLAYABILITIESEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogModularGameplayAbilitiesEditor, Log, All);

#define MGA_EDITOR_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogModularGameplayAbilitiesEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#define MGA_EDITOR_NS_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogModularGameplayAbilitiesEditor, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}