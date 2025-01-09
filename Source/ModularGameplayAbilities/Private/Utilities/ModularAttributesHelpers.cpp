// Copyright Halcyonyx Studios.

#include "Utilities/ModularAttributesHelpers.h"

#include "Attributes/ModularAttributeSetBase.h"


FString UModularAttributesHelpers::GetDebugStringFromExecutionData(const FMGAAttributeSetExecutionData& ExecutionData)
{
	return ExecutionData.ToString(LINE_TERMINATOR);
}

FString UModularAttributesHelpers::GetDebugStringFromExecutionDataSeparator(const FMGAAttributeSetExecutionData& ExecutionData, const FString& Separator)
{
	return ExecutionData.ToString(Separator);
}

FString UModularAttributesHelpers::GetDebugStringFromAttribute(const FGameplayAttribute& Attribute)
{
	return Attribute.GetName();
}

bool UModularAttributesHelpers::NotEqual_GameplayAttributeGameplayAttribute(const FGameplayAttribute A, const FString B)
{
	return A.GetName() != B;
}

FText UModularAttributesHelpers::GetAttributeDisplayNameText(const FGameplayAttribute& InAttribute)
{
	return FText::FromString(InAttribute.GetName());
}
