#include "ModularAbilityPlayerState.h"

#include "ActorComponent/ModularAbilitySystemComponent.h"
#include "GameplayTagStack.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilityPlayerState)

AModularAbilityPlayerState::AModularAbilityPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ModularAbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UModularAbilitySystemComponent>(this, TEXT("ModularAbilitySystemComponent"));
	ModularAbilitySystemComponent->SetIsReplicated(true);
	ModularAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);
}

void AModularAbilityPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AModularAbilityPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	check(ModularAbilitySystemComponent);
	ModularAbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

}

void AModularAbilityPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, StatTags);
}

void AModularAbilityPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AModularAbilityPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AModularAbilityPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AModularAbilityPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}
