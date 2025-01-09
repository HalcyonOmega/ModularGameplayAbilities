// Copyright Chronicler.

#pragma once

#include "GameplayAbilities/ModularGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "NativeGameplayTags.h"

#include "ModularAbilitySystemComponent.generated.h"

class AActor;
class UGameplayAbility;
class UModularAbilityTagRelationshipMapping;
class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;

MODULARGAMEPLAYABILITIES_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Ability_Input_Blocked);

/* Structure passed down to Actors Blueprint with PostGameplayEffectExecute Event */
USTRUCT(BlueprintType)
struct FModularGameplayEffectExecuteData
{
	GENERATED_BODY()

	/* The owner AttributeSet from which the event was invoked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;

	/* The owner AbilitySystemComponent for this AttributeSet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	/* Calculated DeltaValue from OldValue to NewValue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	float DeltaValue = 0.f;
};

/* State Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FModularOnInitAbilityActorInfo);

/* Ability Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModularOnAbilityActivate, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModularOnAbilityCommit, UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModularOnAbilityEnd, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModularOnAbilityFail, const UGameplayAbility*, Ability, const FGameplayTagContainer&, ReasonTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FModularOnCooldownStart, UGameplayAbility*, Ability, const FGameplayTagContainer, CooldownTags, float, TimeRemaining, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FModularOnCooldownChange, UGameplayAbility*, Ability, const FGameplayTag, CooldownTag, float, TimeRemaining, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModularOnCooldownEnd, UGameplayAbility*, Ability, const FGameplayTag, CooldownTag);

/* Attribute Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModularOnPreAttributeChange, UAttributeSet*, AttributeSet, FGameplayAttribute, Attribute, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModularOnAttributeChange, FGameplayAttribute, Attribute, float, DeltaValue, const struct FGameplayTagContainer, EventTags);

/* Effect Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FModularOnPreGameplayEffectExecute, FGameplayAttribute, Attribute, AActor*, SourceActor, AActor*, TargetActor, const FGameplayTagContainer&, SourceTags, const FModularGameplayEffectExecuteData, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModularOnGameplayEffectAdd, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FModularOnGameplayEffectRemove, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FModularOnGameplayEffectStackChange, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle, int32, NewStackCount, int32, OldStackCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FModularOnGameplayEffectTimeChange, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle, float, NewStartTime, float, NewDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FModularOnPostGameplayEffectExecute, FGameplayAttribute, Attribute, AActor*, SourceActor, AActor*, TargetActor, const FGameplayTagContainer&, SourceTags, const FModularGameplayEffectExecuteData, Payload);

/* Gameplay Tag Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModularOnGameplayTagChange, FGameplayTag, GameplayTag, int32, NewTagCount);

/*
 * Gameplay Feature System component extending the Gameplay Ability System Component with Enhanced Input.
 *
 * Adds activation groups with blockers and cancel tags, and input tags for activating abilities.
 * Requires the ModularAbilityExtensionComponent if you want to bind inputs to the input tags on dynamic load.
 * The GAS component also completely manages tags and tag-related queries for the owning actor,
 * which muddies the separation of concerns.
 * For runtime binding, add to both the ModularPlayerCharacter and ModularPlayerState.
 * Note: ModularAbilityExtensionComponent will automatically add this system component from the possessing player state.
 *
 * @todo Refactor the parent class's tag management into a separate component.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=AbilitySystem, HideCategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), EditInlineNew, meta=(BlueprintSpawnableComponent))
class MODULARGAMEPLAYABILITIES_API UModularAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UModularAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	//~End of UActorComponent interface

	/* Handle Delegates */
	void RegisterDelegates();
	void UnregisterDelegates();
	
	typedef TFunctionRef<bool(const UModularGameplayAbility* ModularAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

	void CancelInputActivatedAbilities(bool bReplicateCancelAbility);

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	bool IsActivationGroupBlocked(EModularAbilityActivationGroup Group) const;
	void AddAbilityToActivationGroup(EModularAbilityActivationGroup Group, UModularGameplayAbility* ModularAbility);
	void RemoveAbilityFromActivationGroup(EModularAbilityActivationGroup Group, UModularGameplayAbility* ModularAbility);
	void CancelActivationGroupAbilities(EModularAbilityActivationGroup Group, UModularGameplayAbility* IgnoreModularAbility, bool bReplicateCancelAbility);

	/* Uses a gameplay effect to add the specified dynamic granted tag. */
	void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/* Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag. */
	void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/* Gets the ability target data associated with the given ability handle and activation info. */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);

	/* Sets the current tag relationship mapping, if null it will clear it out. */
	void SetTagRelationshipMapping(UModularAbilityTagRelationshipMapping* NewMapping);
	
	/* Looks at ability tags and gathers additional required and blocking tags. */
	void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;

	/* Gets the level of gameplay effect based on handle. */
	UFUNCTION(BlueprintCallable)
	int32 GetActiveGameplayEffectLevel(FActiveGameplayEffectHandle ActiveHandle);

	
	/* State Delegates */
	
	/*
	 * Event called just after InitAbilityActorInfo, once abilities and attributes have been granted.
	 *
	 * This will happen multiple times for both client / server:
	 *
	 * - Once for Server after component initialization
	 * - Once for Server after replication of owning actor (Possessed by for Player State)
	 * - Once for Client after component initialization
	 * - Once for Client after replication of owning actor (Once more for Player State OnRep_PlayerState)
	 *
	 * Also depends on whether ASC lives on Pawns or Player States.
	 */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Actor")
	FModularOnInitAbilityActorInfo OnInitAbilityActorInfo;
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	
	/* Ability Delegates */
	
	/* Called when an ability is activated for the owner actor. */
	UPROPERTY(BlueprintAssignable, Category = "ModularAbilitySystem|Ability")
	FModularOnAbilityActivate OnAbilityActivate;
	/* Delegate Callbacks trigger corresponding event in Pawn. */
	virtual void HandleOnAbilityActivate(UGameplayAbility* Ability); 
		
	/* Called whenever an ability is committed (cost / cooldown are applied). */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Ability")
	FModularOnAbilityCommit OnAbilityCommit;
	/* Trigger by ASC when an ability is committed (cost / cooldown are applied). */
	void HandleOnAbilityCommit(UGameplayAbility *ActivatedAbility);
	
	/* Called when an ability is ended for the owner actor. */
	UPROPERTY(BlueprintAssignable, Category = "ModularAbilitySystem|Ability")
	FModularOnAbilityEnd OnAbilityEnd;
	/* Delegate Callbacks trigger corresponding event in Pawn. */
	virtual void HandleOnAbilityEnd(UGameplayAbility* Ability);
	
	/* Called when an ability failed to activated for the owner actor, passes along the failed ability
	* and a tag explaining why. */
	UPROPERTY(BlueprintAssignable, Category = "ModularAbilitySystem|Ability")
	FModularOnAbilityFail OnAbilityFail;
	/* Delegate Callbacks trigger corresponding event in Pawn. */
	virtual void HandleOnAbilityFail(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags);

	/* Called when an ability with a valid cooldown is committed and cooldown is applied. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Ability")
	FModularOnCooldownStart OnCooldownStart;
	/* Delegate callback when cooldown starts. */
	void HandleOnCooldownStart(UGameplayAbility* Ability, const FGameplayTagContainer& CooldownTags, float TimeRemaining, float Duration);

	/* Called when an ability with a valid cooldown is committed and cooldown is applied. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Ability")
	FModularOnCooldownChange OnCooldownChange;
	/* Delegate callback when cooldown changes. */
	virtual void HandleOnCooldownChange(const FGameplayTag GameplayTag, int32 NewCount, FGameplayAbilitySpecHandle AbilitySpecHandle, float Duration, bool bNewRemove);
	
	/* Called when a cooldown gameplay tag is removed, meaning cooldown expired. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Ability")
	FModularOnCooldownEnd OnCooldownEnd;
	/* Delegate callback when cooldown ends. */
	virtual void HandleOnCooldownEnd(UGameplayAbility* ActivatedAbility, const FGameplayTag CooldownTag);

	
	/* Attribute Delegates */
	
	/*
	* PreAttributeChange event fired off from native AttributeSets, react here to
	* changes of Attributes CurrentValue before the modification to the BaseValue
	* happens.
	*
	* Called just before any modification happens to an attribute, whether using
	* Attribute setters or using GameplayEffect.
	*
	* @param AttributeSet The AttributeSet that started the change
	* @param Attribute The affected GameplayAttribute
	* @param NewValue The new value
	*/
	UPROPERTY(BlueprintAssignable, Category = "ModularAbilitySystem|Attribute")
	FModularOnPreAttributeChange OnPreAttributeChange;
	virtual void HandlePreAttributeChange(UAttributeSet* AttributeSet, const FGameplayAttribute& Attribute, float NewValue);

	/*
	* Called when any of the attributes owned by this character are changed
	*
	* @param Attribute The Attribute that was changed
	* @param DeltaValue It was an additive operation, returns the modifier value.
	*                   Or if it was a change coming from damage meta attribute (for health),
	*                   returns the damage done.
	* @param EventTags The gameplay tags of the event that changed this attribute
	*/
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Attribute")
	FModularOnAttributeChange OnAttributeChange;
	/* Generic Attribute change callback. */
	virtual void HandleOnAttributeChange(const FOnAttributeChangeData& Data);

	
	/* Effect Delegates */
	
	/* Called when a GameplayEffect is added. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Effect")
	FModularOnGameplayEffectAdd OnGameplayEffectAdd;
	/* Triggered by ASC when GEs are added. */
	virtual void HandleOnGameplayEffectAdd(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	/* Called when a GameplayEffect is removed. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Effect")
	FModularOnGameplayEffectRemove OnGameplayEffectRemove;
	/* Triggered by ASC when any GEs are removed. */
	virtual void HandleOnGameplayEffectRemove(const FActiveGameplayEffect& EffectRemoved);

	/* Called when a GameplayEffect is added or removed. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Effect")
	FModularOnGameplayEffectStackChange OnGameplayEffectStackChange;
	/* Triggered by ASC when GEs stack count changes. */
	virtual void HandleOnGameplayEffectStackChange(FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 PreviousStackCount);

	/* Called when a GameplayEffect duration is changed (for instance when duration is refreshed). */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Effect")
	FModularOnGameplayEffectTimeChange OnGameplayEffectTimeChange;
	/* Triggered by ASC when GEs stack count changes. */
	virtual void HandleOnGameplayEffectTimeChange(FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);
	
	/*
	* PostGameplayEffectExecute event fired off from native AttributeSets, define here
	* any attribute change specific management you are not doing in c++ (like clamp)
	*
	* Only triggers after changes to the BaseValue of an Attribute from a GameplayEffect.
	*
	* @param Attribute The affected GameplayAttribute
	* @param SourceActor The instigator Actor that started the whole chain (in case of damage, that would be the damage causer)
	* @param TargetActor The owner Actor to which the Attribute change is applied
	* @param SourceTags The aggregated SourceTags for this EffectSpec
	* @param Payload information with the original AttributeSet, the owning AbilitySystemComponent, calculated DeltaValue and the ClampMinimumValue from config if defined
	*/
	UPROPERTY(BlueprintAssignable, Category = "ModularAbilitySystem|Effect")
	FModularOnPostGameplayEffectExecute OnPostGameplayEffectExecute;
	virtual void HandlePostGameplayEffectExecute(UAttributeSet* AttributeSet, const FGameplayEffectModCallbackData& Data);

	
	/* Gameplay Tag Delegates */

	/* Called when a Gameplay Tag Changes. */
	UPROPERTY(BlueprintAssignable, Category="ModularAbilitySystem|Tags")
	FModularOnGameplayTagChange OnGameplayTagChange;
	virtual void HandleOnGameplayTagChange(const FGameplayTag GameplayTag, const int32 NewCount);


	
	/* Returns the current value of an attribute (base value). That is, the value of the attribute with no stateful modifiers. */
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Attribute")
	virtual float GetAttributeBaseValue(FGameplayAttribute Attribute) const;

	/* Returns current (final) value of an attribute. */
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Attribute")
	virtual float GetAttributeCurrentValue(FGameplayAttribute Attribute) const;
	
	/*
	* Grants the Actor with the given ability, making it available for activation
	*
	* @param Ability The Gameplay Ability to give to the character
	* @param Level The Gameplay Ability Level (defaults to 1)
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ModularAbilitySystem|Ability")
	virtual void GrantAbility(TSubclassOf<UGameplayAbility> Ability, int32 Level = 1);
	
	/*
	* Remove an ability from the Actor's Ability System Component
	*
	* @param Ability The Gameplay Ability Class to remove
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ModularAbilitySystem|Ability")
	virtual void RemoveAbility(TSubclassOf<UGameplayAbility> Ability);

	/*
	* Remove set of abilities from the Actor's Ability System Component
	*
	* @param Abilities Array of Ability Class to remove
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ModularAbilitySystem|Ability")
	virtual void RemoveAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities);

	/* Returns whether one of the actor's active abilities are matching the provided Ability Class. */
	UFUNCTION(BlueprintPure, Category="ModularAbilitySystem|Ability")
	bool IsUsingAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass);

	/* Returns whether one of the character's active abilities are matching the provided tags. */
	UFUNCTION(BlueprintPure, Category="ModularAbilitySystem|Ability")
	bool IsUsingAbilityByTags(FGameplayTagContainer AbilityTags);

	/*
	* Returns a list of currently active ability instances that match the given class
	*
	* @param AbilityToSearch The Gameplay Ability Class to search for
	*/
	UFUNCTION(BlueprintCallable, Category="ModularAbilitySystem|Ability")
	TArray<UGameplayAbility*> GetActiveAbilitiesByClass(TSubclassOf<UGameplayAbility> AbilityToSearch) const;

	/*
	* Returns a list of currently active ability instances that match the given tags
	*
	* This only returns if the ability is currently running
	*
	* @param GameplayTagContainer The Ability Tags to search for
	*/
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Ability")
	virtual TArray<UGameplayAbility*> GetActiveAbilitiesByTags(const FGameplayTagContainer GameplayTagContainer) const;

	/* Sets the base value of an attribute. Existing active modifiers are NOT cleared and will act upon the new base value. */
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Attribute")
	virtual void SetAttributeBaseValue(FGameplayAttribute Attribute, float NewValue);

	/* Clamps the Attribute from MinValue to MaxValue. */
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Attribute")
	virtual void ClampAttributeBaseValue(FGameplayAttribute Attribute, float MinValue, float MaxValue);

	/*
	* Helper function to proportionally adjust the value of an attribute when it's associated max attribute changes.
	* (e.g. When MaxHealth increases, Health increases by an amount that maintains the same percentage as before)
	*
	* @param AttributeSet The AttributeSet owner for the affected attributes
	* @param AffectedAttributeProperty The affected Attribute property
	* @param MaxAttribute The related MaxAttribute
	* @param NewMaxValue The new value for the MaxAttribute
	*/
	UFUNCTION(BlueprintCallable, Category = "ModularAbilitySystem|Attribute")
	virtual void AdjustAttributeForMaxChange(UPARAM(ref) UAttributeSet* AttributeSet, const FGameplayAttribute AffectedAttributeProperty, const FGameplayAttribute MaxAttribute, float NewMaxValue);

protected:

	void TryActivateAbilitiesOnSpawn();

	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	virtual void NotifyAbilityActivated(
		const FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability) override;
	virtual void NotifyAbilityFailed(
		const FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability,
		const FGameplayTagContainer& FailureReason) override;
	virtual void NotifyAbilityEnded(
		FGameplayAbilitySpecHandle Handle,
		UGameplayAbility* Ability,
		bool bWasCancelled) override;
	virtual void ApplyAbilityBlockAndCancelTags(
		const FGameplayTagContainer& AbilityTags,
		UGameplayAbility* RequestingAbility,
		bool bEnableBlockTags,
		const FGameplayTagContainer& BlockTags,
		bool bExecuteCancelTags,
		const FGameplayTagContainer& CancelTags) override;
	virtual void HandleChangeAbilityCanBeCanceled(
		const FGameplayTagContainer& AbilityTags,
		UGameplayAbility* RequestingAbility,
		bool bCanBeCanceled) override;

	/* Notify client that an ability failed to activate. */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
	
public:
	/* List of GameplayEffects to apply when the Ability System Component is initialized (typically on begin play). */
	UPROPERTY(EditDefaultsOnly, Category = "ModularAbilitySystem|Effect")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;

protected:
	/* If set, this table is used to look up tag relationships for activate and cancel. */
	UPROPERTY()
	TObjectPtr<UModularAbilityTagRelationshipMapping> TagRelationshipMapping;

	/* Handles to abilities that had their input pressed this frame. */
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	/* Handles to abilities that had their input released this frame. */
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	/* Handles to abilities that have their input held. */
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	/* Number of abilities running in each activation group. */
	int32 ActivationGroupCounts[static_cast<uint8>(EModularAbilityActivationGroup::MAX)];
	
	/* Cached applied Startup Effects. */
	UPROPERTY(transient)
	TArray<FActiveGameplayEffectHandle> AddedEffects;
	
	/* Called when Ability System Component is initialized. */
	void GrantStartupEffects();
	
private:
	/* Array of active GE handle bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	/* Array of tags bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FGameplayTag> GameplayTagHandles;

public:

	/* Returns the aggregated SourceTags for this EffectSpec. */
	virtual const FGameplayTagContainer& GetSourceTagsFromContext(const FGameplayEffectModCallbackData& Data);

	/* Templated version of GetCharactersFromContext. */
	template <class TReturnType>
	void GetSourceAndTargetFromContext(const FGameplayEffectModCallbackData& Data, TReturnType*& SourceActor, TReturnType*& TargetActor)
	{
		const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
		UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();

		// Get the Target actor, which should be our owner
		if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
		{
			TargetActor = Cast<TReturnType>(Data.Target.AbilityActorInfo->AvatarActor.Get());
		}

		// Get the Source actor, which should be the damage causer (instigator)
		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			// Set the source actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Cast<TReturnType>(Context.GetEffectCauser());
			}
			else
			{
				SourceActor = Cast<TReturnType>(Source->AbilityActorInfo->AvatarActor.Get());
			}
		}
	}	
};
