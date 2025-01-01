// Copyright Chronicler.

#pragma once

#include "GameplayAbilities/ModularGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

#include "ModularAbilitySystemComponent.generated.h"

class AActor;
class UGameplayAbility;
class UModularAbilityTagRelationshipMapping;
class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;

MODULARGAMEPLAYABILITIES_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Ability_Input_Blocked);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FModularOnInitAbilityActorInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModularOnAbilityActivated, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FModularOnAbilityEnded, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FModularOnAbilityFailed, const UGameplayAbility*, Ability, const FGameplayTagContainer&, ReasonTags);

/**
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
	//~End of UActorComponent interface

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

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

	// Uses a gameplay effect to add the specified dynamic granted tag.
	void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);

	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);

	/** Gets the ability target data associated with the given ability handle and activation info */
	void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);

	/** Sets the current tag relationship mapping, if null it will clear it out */
	void SetTagRelationshipMapping(UModularAbilityTagRelationshipMapping* NewMapping);
	
	/** Looks at ability tags and gathers additional required and blocking tags */
	void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;

	UFUNCTION(BlueprintCallable)
	int32 GetActiveGameplayEffectLevel(FActiveGameplayEffectHandle ActiveHandle);

	/**
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
	UPROPERTY(BlueprintAssignable, Category="Abilities")
	FModularOnInitAbilityActorInfo OnInitAbilityActorInfo;

	
	/* Called when an ability is activated for the owner actor. */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FModularOnAbilityActivated OnAbilityActivated;

	/* Called when an ability is ended for the owner actor. */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FModularOnAbilityEnded OnAbilityEnded;

	/* Called when an ability failed to activated for the owner actor, passes along the failed ability
	* and a tag explaining why. */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FModularOnAbilityFailed OnAbilityFailed;
	
	//~ Those are Delegate Callbacks register with this ASC to trigger corresponding events on the Owning Character (mainly for ability queuing)
	virtual void OnAbilityActivatedCallback(UGameplayAbility* Ability);
	virtual void OnAbilityFailedCallback(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags);
	virtual void OnAbilityEndedCallback(UGameplayAbility* Ability);
	
	/** List of GameplayEffects to apply when the Ability System Component is initialized (typically on begin play) */
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;
	
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

	/** Notify client that an ability failed to activate */
	UFUNCTION(Client, Unreliable)
	void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);

	void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
protected:

	// If set, this table is used to look up tag relationships for activate and cancel
	UPROPERTY()
	TObjectPtr<UModularAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Handles to abilities that had their input pressed this frame.
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// Handles to abilities that had their input released this frame.
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// Handles to abilities that have their input held.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	// Number of abilities running in each activation group.
	int32 ActivationGroupCounts[static_cast<uint8>(EModularAbilityActivationGroup::MAX)];
	
	// Cached applied Startup Effects
	UPROPERTY(transient)
	TArray<FActiveGameplayEffectHandle> AddedEffects;
	
	/** Called when Ability System Component is initialized */
	void GrantStartupEffects();
};
