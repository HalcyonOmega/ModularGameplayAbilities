// Copyright 2022-2024 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

MODULARGAMEPLAYABILITIESSCAFFOLD_API DECLARE_LOG_CATEGORY_EXTERN(LogModularGameplayAbilitiesScaffold, Display, All);

#define MGA_SCAFFOLD_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogModularGameplayAbilitiesScaffold, Verbosity, Format, ##__VA_ARGS__); \
}
