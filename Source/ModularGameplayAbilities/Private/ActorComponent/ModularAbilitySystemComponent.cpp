// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActorComponent/ModularAbilitySystemComponent.h"

#include "ModularGameplayAbilitiesLogChannels.h"
#include "Animation/GameplayTagsAnimInstance.h"
#include "DataAsset/ModularAbilityData.h"
#include "DataAsset/ModularAssetManager.h"
#include "GameplayAbilities/ModularGameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayAbilities/ModularAbilityTagRelationshipMapping.h"
#include "GameplayAbilities/ModularGlobalAbilitySystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Ability_Input_Blocked, "Gameplay.Ability.Input.Blocked");

UModularAbilitySystemComponent::UModularAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UModularAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	
	RegisterDelegates();

	// Grant startup effects on begin play instead of from within InitAbilityActorInfo to avoid
	// "ticking" periodic effects when BP is first opened
	GrantStartupEffects();
}

void UModularAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UModularGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UModularGlobalAbilitySystem>(GetWorld()))
	{
		GlobalAbilitySystem->UnregisterAbilityComponent(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UModularAbilitySystemComponent::BeginDestroy()
{
	UnregisterDelegates();

	Super::BeginDestroy();
}

void UModularAbilitySystemComponent::RegisterDelegates()
{
	UE_LOG(LogModularGameplayAbilities, Verbose, TEXT("Registering Delegates for %s"), *GetNameSafe(this));

	UnregisterDelegates();

	/* Ability Delegates */
	AbilityActivatedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::HandleOnAbilityActivate);
	AbilityCommittedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::HandleOnAbilityCommit);
	AbilityEndedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::HandleOnAbilityEnd);
	AbilityFailedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::HandleOnAbilityFail);

	/* Attribute Delegates */
	TArray<FGameplayAttribute> Attributes;
	GetAllAttributes(Attributes);
	
	for (const FGameplayAttribute Attribute : Attributes)
	{
		GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UModularAbilitySystemComponent::HandleOnAttributeChange);
	}

	/* Effect Delegates*/
	OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UModularAbilitySystemComponent::HandleOnGameplayEffectAdd);
	OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UModularAbilitySystemComponent::HandleOnGameplayEffectRemove);

	/* Gameplay Tag Delegates */
	RegisterGenericGameplayTagEvent().AddUObject(this, &UModularAbilitySystemComponent::HandleOnGameplayTagChange);
}

void UModularAbilitySystemComponent::UnregisterDelegates()
{
	/* Ability Delegates */
	AbilityActivatedCallbacks.RemoveAll(this);
	AbilityCommittedCallbacks.RemoveAll(this);
	AbilityEndedCallbacks.RemoveAll(this);
	AbilityFailedCallbacks.RemoveAll(this);

	/* Attribute Delegates */
	TArray<FGameplayAttribute> Attributes;
	GetAllAttributes(Attributes);
	
	for (const FGameplayAttribute Attribute : Attributes)
	{
		GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
	}

	/* Effect Delegates*/
	OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
	OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);

	for (const FActiveGameplayEffectHandle GameplayEffectHandle : GameplayEffectHandles)
	{
		if (FOnActiveGameplayEffectStackChange* EffectStackChangeDelegate = OnGameplayEffectStackChangeDelegate(GameplayEffectHandle))
		{
			EffectStackChangeDelegate->RemoveAll(this);
		}

		if (FOnActiveGameplayEffectTimeChange* EffectTimeChangeDelegate = OnGameplayEffectTimeChangeDelegate(GameplayEffectHandle))
		{
			EffectTimeChangeDelegate->RemoveAll(this);
		}
	}
	
	/* Gameplay Tag Delegates */
	RegisterGenericGameplayTagEvent().RemoveAll(this);

	for (const FGameplayTag GameplayTagBoundDelegate : GameplayTagHandles)
	{
		RegisterGameplayTagEvent(GameplayTagBoundDelegate).RemoveAll(this);
	}
}

void UModularAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// Notify all abilities that a new pawn avatar has been set
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			for (TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
				UGameplayAbility* AbilityInstance : Instances)
			{
				if (UModularGameplayAbility* ModularAbilityInstance = Cast<UModularGameplayAbility>(AbilityInstance))
				{
					// Ability instances may be missing for replays.
					ModularAbilityInstance->OnPawnAvatarSet();
				}
			}
		}

		// Register with the global system once we actually have a pawn avatar.
		// We wait until this time since some globally-applied effects may require an avatar.
		if (UModularGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UModularGlobalAbilitySystem>(GetWorld()))
		{
			GlobalAbilitySystem->RegisterAbilityComponent(this);
		}

		if (UGameplayTagsAnimInstance* ModularAnimInst = Cast<UGameplayTagsAnimInstance>(ActorInfo->GetAnimInstance()))
		{
			ModularAnimInst->InitializeWithAbilitySystem(this);
		}
		
		/* This will happen multiple times for both client/server */
		OnInitAbilityActorInfo.Broadcast();
		
		TryActivateAbilitiesOnSpawn();
	}
}

void UModularAbilitySystemComponent::HandleOnAbilityActivate(UGameplayAbility* Ability)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityActivatedCallback %s"), *Ability->GetName());
	/* @TODO: What is the value of checking Avatar Actor? What if it's a Player State or Global Ability?
	 * const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("UModularAbilitySystemComponent::OnAbilityActivated No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	} */
	
	OnAbilityActivate.Broadcast(Ability);
}

void UModularAbilitySystemComponent::HandleOnAbilityCommit(UGameplayAbility* ActivatedAbility)
{
	if (!ActivatedAbility) {return;}
	
	OnAbilityCommit.Broadcast(ActivatedAbility);

	/* Handle cooldown */
	if (UGameplayEffect* CooldownGE = ActivatedAbility->GetCooldownGameplayEffect(); !CooldownGE) {return;}

	if (!ActivatedAbility->IsInstantiated()) {return;}

	const FGameplayTagContainer* CooldownTags = ActivatedAbility->GetCooldownTags();
	if (!CooldownTags || CooldownTags->Num() <= 0) {return;}

	FGameplayAbilityActorInfo ActorInfo = ActivatedAbility->GetActorInfo();
	const FGameplayAbilitySpecHandle AbilitySpecHandle = ActivatedAbility->GetCurrentAbilitySpecHandle();

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	ActivatedAbility->GetCooldownTimeRemainingAndDuration(AbilitySpecHandle, &ActorInfo, TimeRemaining, Duration);

	HandleOnCooldownStart(ActivatedAbility, *CooldownTags, TimeRemaining, Duration);

	/* Register delegate to monitor any change to cooldown gameplay tag to be able to figure out when a cooldown expires. */
	TArray<FGameplayTag> GameplayTags;
	CooldownTags->GetGameplayTagArray(GameplayTags);
	
	for (const FGameplayTag GameplayTag : GameplayTags)
	{
		RegisterGameplayTagEvent(GameplayTag, EGameplayTagEventType::AnyCountChange).AddUObject(this, &UModularAbilitySystemComponent::HandleOnCooldownChange, AbilitySpecHandle, Duration, true);
		GameplayTagHandles.AddUnique(GameplayTag);
	}
}

void UModularAbilitySystemComponent::HandleOnAbilityEnd(UGameplayAbility* Ability)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityEndedCallback %s"), *Ability->GetName());
	/* @TODO: What is the value of checking Avatar Actor? What if it's a Player State or Global Ability?
	 * const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("UModularAbilitySystemComponent::OnAbilityEndedCallback No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	} */

	OnAbilityEnd.Broadcast(Ability);
}

void UModularAbilitySystemComponent::HandleOnAbilityFail(const UGameplayAbility* Ability,
	const FGameplayTagContainer& Tags)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityFailedCallback %s"), *Ability->GetName());
	/* @TODO: What is the value of checking Avatar Actor? What if it's a Player State or Global Ability?
	 * const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("UModularAbilitySystemComponent::OnAbilityFailed No OwnerActor for this ability: %s Tags: %s"), *Ability->GetName(), *Tags.ToString());
		return;
	} */

	OnAbilityFail.Broadcast(Ability, Tags);
}

void UModularAbilitySystemComponent::HandleOnCooldownStart(UGameplayAbility* Ability, const FGameplayTagContainer& CooldownTags, float TimeRemaining, float Duration)
{
	OnCooldownStart.Broadcast(Ability, CooldownTags, TimeRemaining, Duration);
}

void UModularAbilitySystemComponent::HandleOnCooldownChange(const FGameplayTag GameplayTag, int32 NewCount, FGameplayAbilitySpecHandle AbilitySpecHandle, float Duration, bool bNewRemove)
{
	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec) {return;} /* Ability might have been cleared when cooldown expires. */		

	if (UGameplayAbility* Ability = AbilitySpec->Ability; IsValid(Ability))
	{
		if (NewCount != 0) {OnCooldownChange.Broadcast(Ability, GameplayTag, NewCount, Duration);}
		else {HandleOnCooldownEnd(Ability, GameplayTag);}
	}
}

void UModularAbilitySystemComponent::HandleOnCooldownEnd(UGameplayAbility* ActivatedAbility, const FGameplayTag CooldownTag)
{
	OnCooldownEnd.Broadcast(ActivatedAbility, CooldownTag);
	RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::AnyCountChange).RemoveAll(this);
}

void UModularAbilitySystemComponent::HandlePreAttributeChange(UAttributeSet* AttributeSet,
	const FGameplayAttribute& Attribute, float NewValue)
{
	OnPreAttributeChange.Broadcast(AttributeSet, Attribute, NewValue);
}

void UModularAbilitySystemComponent::HandleOnAttributeChange(const FOnAttributeChangeData& Data)
{
	const float NewValue = Data.NewValue;
	const float OldValue = Data.OldValue;

	/* Prevent broadcast Attribute changes if New and Old values are the same, most likely because of clamping in post gameplay effect execute. */
	if (OldValue == NewValue) {return;}

	const FGameplayEffectModCallbackData* ModData = Data.GEModData;
	FGameplayTagContainer SourceTags = FGameplayTagContainer();
	if (ModData)
	{
		SourceTags = *ModData->EffectSpec.CapturedSourceTags.GetAggregatedTags();
	}

	/* Broadcast attribute change to component. */
	OnAttributeChange.Broadcast(Data.Attribute, NewValue - OldValue, SourceTags);
}

void UModularAbilitySystemComponent::HandleOnGameplayEffectAdd(UAbilitySystemComponent* Target,
	const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
	FGameplayTagContainer AssetTags;
	SpecApplied.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectAdd.Broadcast(AssetTags, GrantedTags, ActiveHandle);

	if (FOnActiveGameplayEffectStackChange* Delegate = OnGameplayEffectStackChangeDelegate(ActiveHandle))
	{
		Delegate->AddUObject(this, &UModularAbilitySystemComponent::HandleOnGameplayEffectStackChange);
	}

	if (FOnActiveGameplayEffectTimeChange* Delegate = OnGameplayEffectTimeChangeDelegate(ActiveHandle))
	{
		Delegate->AddUObject(this, &UModularAbilitySystemComponent::HandleOnGameplayEffectTimeChange);
	}

	GameplayEffectHandles.AddUnique(ActiveHandle);
}

void UModularAbilitySystemComponent::HandleOnGameplayEffectRemove(const FActiveGameplayEffect& EffectRemoved)
{
	FGameplayTagContainer AssetTags;
	EffectRemoved.Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	EffectRemoved.Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectStackChange.Broadcast(AssetTags, GrantedTags, EffectRemoved.Handle, 0, 1);
	OnGameplayEffectRemove.Broadcast(AssetTags, GrantedTags, EffectRemoved.Handle);
}

void UModularAbilitySystemComponent::HandleOnGameplayEffectStackChange(FActiveGameplayEffectHandle ActiveHandle,
	int32 NewStackCount, int32 PreviousStackCount)
{
	const FActiveGameplayEffect* GameplayEffect = GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect) {return;}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectStackChange.Broadcast(AssetTags, GrantedTags, ActiveHandle, NewStackCount, PreviousStackCount);
}

void UModularAbilitySystemComponent::HandleOnGameplayEffectTimeChange(FActiveGameplayEffectHandle ActiveHandle,
	float NewStartTime, float NewDuration)
{
	const FActiveGameplayEffect* GameplayEffect = GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect) {return;}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectTimeChange.Broadcast(AssetTags, GrantedTags, ActiveHandle, NewStartTime, NewDuration);
}

void UModularAbilitySystemComponent::HandlePostGameplayEffectExecute(UAttributeSet* AttributeSet,
	const FGameplayEffectModCallbackData& Data)
{
	if (!AttributeSet)
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("ModularAttributeSet isn't valid"));
		return;
	}

	AActor* SourceActor = nullptr;
	AActor* TargetActor = nullptr;
	GetSourceAndTargetFromContext<AActor>(Data, SourceActor, TargetActor);

	const FGameplayTagContainer SourceTags = GetSourceTagsFromContext(Data);
	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	
	/* Compute the delta between old and new, if it is available. */
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		/* If this was additive, store the raw delta value to be passed along later. */
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	/* Delegate any attribute handling to Blueprints. */
	FModularGameplayEffectExecuteData Payload;
	Payload.AttributeSet = AttributeSet;
	Payload.AbilitySystemComponent = AttributeSet->GetOwningAbilitySystemComponent();
	Payload.DeltaValue = DeltaValue;
	OnPostGameplayEffectExecute.Broadcast(Data.EvaluatedData.Attribute, SourceActor, TargetActor, SourceTags, Payload);
}

void UModularAbilitySystemComponent::HandleOnGameplayTagChange(const FGameplayTag GameplayTag, const int32 NewCount)
{
	OnGameplayTagChange.Broadcast(GameplayTag, NewCount);
}

float UModularAbilitySystemComponent::GetAttributeBaseValue(FGameplayAttribute Attribute) const
{
	if (!Attribute.IsValid())
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("Passed in Attribute is invalid (None). Will return 0.f."))
		return 0.f;
	}

	if (!HasAttributeSetForAttribute(Attribute))
	{
		const UClass* AttributeSet = Attribute.GetAttributeSetClass();
		UE_LOG(
			LogModularGameplayAbilities,
			Error,
			TEXT("Trying to get value of attribute [%s.%s]. %s doesn't seem to be granted to %s. Returning 0.f"),
			*GetNameSafe(AttributeSet),
			*Attribute.GetName(),
			*GetNameSafe(AttributeSet),
			*GetNameSafe(this)
		);

		return 0.f;
	}

	return GetNumericAttributeBase(Attribute);
}

float UModularAbilitySystemComponent::GetAttributeCurrentValue(FGameplayAttribute Attribute) const
{
	if (!Attribute.IsValid())
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("Passed in Attribute is invalid (None). Will return 0.f."))
		return 0.f;
	}

	if (!HasAttributeSetForAttribute(Attribute))
	{
		const UClass* AttributeSet = Attribute.GetAttributeSetClass();
		UE_LOG(
			LogModularGameplayAbilities,
			Error,
			TEXT("Trying to get value of attribute [%s.%s]. %s doesn't seem to be granted to %s. Returning 0.f"),
			*GetNameSafe(AttributeSet),
			*Attribute.GetName(),
			*GetNameSafe(AttributeSet),
			*GetNameSafe(this)
		);

		return 0.f;
	}

	return GetNumericAttribute(Attribute);
}

void UModularAbilitySystemComponent::GrantAbility(TSubclassOf<UGameplayAbility> Ability, int32 Level)
{
	if (!GetOwner() || !Ability) {return;}

	if (!IsOwnerActorAuthoritative())
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("GrantAbility Called on non authority"));
		return;
	}

	FGameplayAbilitySpec Spec;
	Spec.Ability = Ability.GetDefaultObject();

	FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, Level, INDEX_NONE, GetOwner());
	GiveAbility(AbilitySpec);
}

void UModularAbilitySystemComponent::RemoveAbility(TSubclassOf<UGameplayAbility> Ability)
{
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToRemove;
	AbilitiesToRemove.Add(Ability);
	return RemoveAbilities(AbilitiesToRemove);
}

void UModularAbilitySystemComponent::RemoveAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities)
{
	if (!GetOwner() || !IsOwnerActorAuthoritative()) {return;}

	/* Remove any abilities added from a previous call. */
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Abilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	/* Do in two passes so the removal happens after we have the full list. */
	for (const FGameplayAbilitySpecHandle AbilityToRemove : AbilitiesToRemove)
	{
		ClearAbility(AbilityToRemove);
	}

}

bool UModularAbilitySystemComponent::IsUsingAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("IsUsingAbilityByClass: Provided AbilityClass is null"))
		return false;
	}

	return GetActiveAbilitiesByClass(AbilityClass).Num() > 0;
}

bool UModularAbilitySystemComponent::IsUsingAbilityByTags(FGameplayTagContainer AbilityTags)
{
	return GetActiveAbilitiesByTags(AbilityTags).Num() > 0;
}

TArray<UGameplayAbility*> UModularAbilitySystemComponent::GetActiveAbilitiesByClass(
	TSubclassOf<UGameplayAbility> AbilityToSearch) const
{
	TArray<FGameplayAbilitySpec> Specs = GetActivatableAbilities();
	TArray<struct FGameplayAbilitySpec*> MatchingGameplayAbilities;
	TArray<UGameplayAbility*> ActiveAbilities;

	// First, search for matching Abilities for this class
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.Ability && Spec.Ability->GetClass()->IsChildOf(AbilityToSearch))
		{
			MatchingGameplayAbilities.Add(const_cast<FGameplayAbilitySpec*>(&Spec));
		}
	}

	// Iterate the list of all ability specs
	for (const FGameplayAbilitySpec* Spec : MatchingGameplayAbilities)
	{
		// Iterate all instances on this ability spec, which can include instance per execution abilities
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			if (ActiveAbility->IsActive())
			{
				ActiveAbilities.Add(ActiveAbility);
			}
		}
	}

	return ActiveAbilities;
}

TArray<UGameplayAbility*> UModularAbilitySystemComponent::GetActiveAbilitiesByTags(
	const FGameplayTagContainer GameplayTagContainer) const
{
	TArray<UGameplayAbility*> ActiveAbilities;
	TArray<FGameplayAbilitySpec*> MatchingGameplayAbilities;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, MatchingGameplayAbilities, false);

	// Iterate the list of all ability specs
	for (const FGameplayAbilitySpec* Spec : MatchingGameplayAbilities)
	{
		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			if (ActiveAbility->IsActive())
			{
				ActiveAbilities.Add(ActiveAbility);
			}
		}
	}

	return ActiveAbilities;
}

void UModularAbilitySystemComponent::SetAttributeBaseValue(FGameplayAttribute Attribute, float NewValue)
{
	SetNumericAttributeBase(Attribute, NewValue);
}

void UModularAbilitySystemComponent::ClampAttributeBaseValue(FGameplayAttribute Attribute, float MinValue, float MaxValue)
{
	const float CurrentValue = GetAttributeBaseValue(Attribute);
	const float NewValue = FMath::Clamp(CurrentValue, MinValue, MaxValue);
	SetAttributeBaseValue(Attribute, NewValue);
}

void UModularAbilitySystemComponent::AdjustAttributeForMaxChange(UAttributeSet* AttributeSet,
	const FGameplayAttribute AffectedAttributeProperty, const FGameplayAttribute MaxAttribute, float NewMaxValue)
{
	FGameplayAttributeData* AttributeData = AffectedAttributeProperty.GetGameplayAttributeData(AttributeSet);
	if (!AttributeData)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("AdjustAttributeForMaxChange() AttributeData returned by AffectedAttributeProperty.GetGameplayAttributeData() seems to be invalid."))
		return;
	}

	const FGameplayAttributeData* MaxAttributeData = MaxAttribute.GetGameplayAttributeData(AttributeSet);
	if (!AttributeData)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("AdjustAttributeForMaxChange() MaxAttributeData returned by MaxAttribute.GetGameplayAttributeData() seems to be invalid."))
		return;
	}

	const float CurrentMaxValue = (*MaxAttributeData).GetCurrentValue();
	const float CurrentValue = (*AttributeData).GetCurrentValue();

	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && CurrentMaxValue > 0.f)
	{
		// Get the current relative percent
		const float Ratio = CurrentValue / CurrentMaxValue;

		// Calculate value for the affected attribute based on current ratio
		const float NewValue = FMath::RoundToFloat(NewMaxValue * Ratio);

		UE_LOG(LogModularGameplayAbilities, Verbose, TEXT("AdjustAttributeForMaxChange: CurrentValue: %f, CurrentMaxValue: %f, NewMaxValue: %f, NewValue: %f (Ratio: %f)"), CurrentValue, CurrentMaxValue, NewMaxValue, NewValue, Ratio)
		UE_LOG(LogModularGameplayAbilities, Verbose, TEXT("AdjustAttributeForMaxChange: ApplyModToAttribute %s with %f"), *AffectedAttributeProperty.GetName(), NewValue)
		ApplyModToAttribute(AffectedAttributeProperty, EGameplayModOp::Override, NewValue);
	}
}

void UModularAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const UModularGameplayAbility* ModularAbilityCDO = CastChecked<UModularGameplayAbility>(AbilitySpec.Ability))
		{
			ModularAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void UModularAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}
		
		UModularGameplayAbility* ModularAbilityCDO = CastChecked<UModularGameplayAbility>(AbilitySpec.Ability); 
		if (!ModularAbilityCDO)
		{
			UE_LOG(LogModularGameplayAbilities, Error, TEXT("CancelAbilitiesByFunc: Non-ModularGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
			continue;
		}

		// Cancel all the spawned instances.
		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			UModularGameplayAbility* ModularAbilityInstance = CastChecked<UModularGameplayAbility>(AbilityInstance);
			
			if (ShouldCancelFunc(ModularAbilityInstance, AbilitySpec.Handle))
			{
				if (ModularAbilityInstance->CanBeCanceled())
				{
					ModularAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), ModularAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
				}
				else
				{
					UE_LOG(LogModularGameplayAbilities, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *ModularAbilityInstance->GetName());
				}
			}
		}
	}
}

void UModularAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this](const UModularGameplayAbility* ModularAbility, FGameplayAbilitySpecHandle Handle)
	{
		const EModularAbilityActivationPolicy ActivationPolicy = ModularAbility->GetActivationPolicy();
		return ((ActivationPolicy == EModularAbilityActivationPolicy::OnInputTriggered)
			|| (ActivationPolicy == EModularAbilityActivationPolicy::WhileInputActive));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UModularAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputPressed event. This is not replicated here.
		// If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.GetPrimaryInstance()->GetCurrentActivationInfo().GetActivationPredictionKey());
	}
}

void UModularAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputReleased event. This is not replicated here.
		// If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Spec.GetPrimaryInstance()->GetCurrentActivationInfo().GetActivationPredictionKey());
	}
}

void UModularAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void UModularAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void UModularAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (HasMatchingGameplayTag(TAG_Gameplay_Ability_Input_Blocked))
	{
		ClearAbilityInput();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//@TODO: See if we can use FScopedServerAbilityRPCBatcher ScopedRPCBatcher in some of these loops

	//
	// Process all abilities that activate when the input is held.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UModularGameplayAbility* ModularAbilityCDO = CastChecked<UModularGameplayAbility>(AbilitySpec->Ability);

				if (ModularAbilityCDO->GetActivationPolicy() == EModularAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//Process all abilities that had their input pressed this frame.
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UModularGameplayAbility* ModularAbilityCDO = CastChecked<UModularGameplayAbility>(AbilitySpec->Ability);

					if (ModularAbilityCDO->GetActivationPolicy() == EModularAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send an input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// Process all abilities that had their input released this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// Clear the cached ability handles.
	//
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UModularAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UModularAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);
	
	if (UModularGameplayAbility* ModularAbility = Cast<UModularGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(ModularAbility->GetActivationGroup(), ModularAbility);
	}
}

void UModularAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
	{
		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
		{
			ClientNotifyAbilityFailed(Ability, FailureReason);
			return;
		}
	}

	HandleAbilityFailed(Ability, FailureReason);
}

void UModularAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	UModularGameplayAbility* ModularAbility = CastChecked<UModularGameplayAbility>(Ability);

	RemoveAbilityFromActivationGroup(ModularAbility->GetActivationGroup(), ModularAbility);
}

void UModularAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	FGameplayTagContainer ModifiedBlockTags = BlockTags;
	FGameplayTagContainer ModifiedCancelTags = CancelTags;

	if (TagRelationshipMapping)
	{
		// Use the mapping to expand the ability tags into block and cancel tag
		TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
	}

	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags, bExecuteCancelTags, ModifiedCancelTags);

	//@TODO: Apply any special logic like blocking input or movement
}

void UModularAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	//@TODO: Apply any special logic like blocking input or movement
}

void UModularAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
{
	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
	}
}

void UModularAbilitySystemComponent::SetTagRelationshipMapping(UModularAbilityTagRelationshipMapping* NewMapping)
{
	TagRelationshipMapping = NewMapping;
}

void UModularAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	HandleAbilityFailed(Ability, FailureReason);
}

void UModularAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	UE_LOG(LogModularGameplayAbilities, Warning, TEXT("Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

	if (const UModularGameplayAbility* ModularAbility = Cast<const UModularGameplayAbility>(Ability))
	{
		ModularAbility->OnAbilityFailedToActivate(FailureReason);
	}	
}

bool UModularAbilitySystemComponent::IsActivationGroupBlocked(EModularAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case EModularAbilityActivationGroup::Independent:
		// Independent abilities are never blocked.
		bBlocked = false;
		break;

	case EModularAbilityActivationGroup::Exclusive_Replaceable:
	case EModularAbilityActivationGroup::Exclusive_Blocking:
		// Exclusive abilities can activate if nothing is blocking.
		bBlocked = (ActivationGroupCounts[StaticCast<uint8>(EModularAbilityActivationGroup::Exclusive_Blocking)] > 0);
		break;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), StaticCast<uint8>(Group));
		break;
	}

	return bBlocked;
}

void UModularAbilitySystemComponent::AddAbilityToActivationGroup(EModularAbilityActivationGroup Group, UModularGameplayAbility* ModularAbility)
{
	check(ModularAbility);
	check(ActivationGroupCounts[StaticCast<uint8>(Group)] < INT32_MAX);

	ActivationGroupCounts[StaticCast<uint8>(Group)]++;

	constexpr bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case EModularAbilityActivationGroup::Independent:
		// Independent abilities do not cancel any other abilities.
		break;

	case EModularAbilityActivationGroup::Exclusive_Replaceable:
	case EModularAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(EModularAbilityActivationGroup::Exclusive_Replaceable, ModularAbility, bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), StaticCast<uint8>(Group));
		break;
	}

	if (const int32 ExclusiveCount = ActivationGroupCounts[StaticCast<uint8>(EModularAbilityActivationGroup::Exclusive_Replaceable)]
		+ ActivationGroupCounts[StaticCast<uint8>(EModularAbilityActivationGroup::Exclusive_Blocking)];
		!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UModularAbilitySystemComponent::RemoveAbilityFromActivationGroup(EModularAbilityActivationGroup Group, UModularGameplayAbility* ModularAbility)
{
	check(ModularAbility);
	check(ActivationGroupCounts[StaticCast<uint8>(Group)] > 0);

	ActivationGroupCounts[StaticCast<uint8>(Group)]--;
}

void UModularAbilitySystemComponent::CancelActivationGroupAbilities(EModularAbilityActivationGroup Group, UModularGameplayAbility* IgnoreModularAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this, Group, IgnoreModularAbility](const UModularGameplayAbility* ModularAbility, FGameplayAbilitySpecHandle Handle)
	{
		return ((ModularAbility->GetActivationGroup() == Group) && (ModularAbility != IgnoreModularAbility));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UModularAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UModularAssetManager::GetSubclass(UModularAbilityData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UModularAbilityData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
		return;
	}

	Spec->DynamicGrantedTags.AddTag(Tag);

	ApplyGameplayEffectSpecToSelf(*Spec);
}

void UModularAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UModularAssetManager::GetSubclass(UModularAbilityData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *UModularAbilityData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	Query.EffectDefinition = DynamicTagGE;

	RemoveActiveEffects(Query);
}

void UModularAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	if (const TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
		ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
	}
}

int32 UModularAbilitySystemComponent::GetActiveGameplayEffectLevel(FActiveGameplayEffectHandle ActiveHandle)
{
	const FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(ActiveHandle);
	if (ActiveEffect)
	{
		return int32(ActiveEffect->Spec.GetLevel());
	}
	return int32(-1);
}

void UModularAbilitySystemComponent::GrantStartupEffects()
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	// Reset/Remove effects if we had already added them
	for (const FActiveGameplayEffectHandle AddedEffect : AddedEffects)
	{
		RemoveActiveGameplayEffect(AddedEffect);
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddSourceObject(this);

	AddedEffects.Empty(GrantedEffects.Num());

	for (const TSubclassOf<UGameplayEffect>& GameplayEffect : GrantedEffects)
	{
		FGameplayEffectSpecHandle NewHandle = MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), this);
			AddedEffects.Add(EffectHandle);
		}
	}
}

const FGameplayTagContainer& UModularAbilitySystemComponent::GetSourceTagsFromContext(
	const FGameplayEffectModCallbackData& Data)
{
	return *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
}
