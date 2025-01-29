#pragma once
#include "Containers/Map.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "GameplayAttributeTag.generated.h"

USTRUCT(BlueprintType)
struct FGameplayAttributeTag : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute Attribute;

	FGameplayAttributeTag()
	{
		
	}
};