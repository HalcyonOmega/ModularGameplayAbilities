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
	
	AbilityActivatedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::OnAbilityActivatedCallback);
	AbilityFailedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::OnAbilityFailedCallback);
	AbilityEndedCallbacks.AddUObject(this, &UModularAbilitySystemComponent::OnAbilityEndedCallback);

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
		
		// For PlayerState client pawns, setup and update owner on companion components if pawns have them
		/* @TODO: UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(InAvatarActor);
		if (CoreComponent)
		{
			CoreComponent->SetupOwner();
			CoreComponent->RegisterAbilitySystemDelegates(this);
			CoreComponent->SetStartupAbilitiesGranted(true);
		}*/
		
		/* Broadcast to Blueprint InitAbilityActorInfo was called
		 *
		 * This will happen multiple times for both client / server */
		OnInitAbilityActorInfo.Broadcast();
		
		TryActivateAbilitiesOnSpawn();
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
	// and then also send a input event to the ability because of the press.
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

void UModularAbilitySystemComponent::OnAbilityActivatedCallback(UGameplayAbility* Ability)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityActivatedCallback %s"), *Ability->GetName());
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Error, TEXT("UModularAbilitySystemComponent::OnAbilityActivated No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	}

	/* @TODO:
	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityActivated.Broadcast(Ability);
		
	}
	*/
	OnAbilityActivated.Broadcast(Ability);
}

void UModularAbilitySystemComponent::OnAbilityFailedCallback(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityFailedCallback %s"), *Ability->GetName());

	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("UModularAbilitySystemComponent::OnAbilityFailed No OwnerActor for this ability: %s Tags: %s"), *Ability->GetName(), *Tags.ToString());
		return;
	}

	OnAbilityFailed.Broadcast(Ability, Tags);
	/* @TODO: 
	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	UGSCAbilityQueueComponent* AbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityFailed.Broadcast(Ability, Tags);
	}

	if (AbilityQueueComponent)
	{
		AbilityQueueComponent->OnAbilityFailed(Ability, Tags);
	}
	*/
}

void UModularAbilitySystemComponent::OnAbilityEndedCallback(UGameplayAbility* Ability)
{
	UE_LOG(LogModularGameplayAbilities, Log, TEXT("UModularAbilitySystemComponent::OnAbilityEndedCallback %s"), *Ability->GetName());
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		UE_LOG(LogModularGameplayAbilities, Warning, TEXT("UModularAbilitySystemComponent::OnAbilityEndedCallback No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	}

	OnAbilityEnded.Broadcast(Ability);
	/*
	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	UGSCAbilityQueueComponent* AbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityEnded.Broadcast(Ability);
	}

	if (AbilityQueueComponent)
	{
		AbilityQueueComponent->OnAbilityEnded(Ability);
	}
	*/
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
