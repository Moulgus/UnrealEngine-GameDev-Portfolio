// AttributeSet: declares replicated vital stats (Health/Mana) and a transient meta attribute (IncomingDamage).
// Works with GAS (Gameplay Ability System): attributes are modified by GameplayEffects and processed on the server.

#pragma once

// Convenience macro: generates the standard GAS accessors for an attribute.
// - FGameplayAttribute (property getter used by effects)
// - Value getter/setter/init for FGameplayAttributeData (runtime access)
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class AURA_API UAuraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	/*
	* Vital Stats
	* Replicated to clients to keep UI and gameplay in sync.
	*/

	// Current Health value. Rep-notify triggers client-side updates (UI, effects).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Vitals")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UAuraAttributeSet, Health)
	// Called when Health changes on clients to propagate GAS notifications.
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	// Current Mana value. Rep-notify triggers client-side updates (UI, effects).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Vitals")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UAuraAttributeSet, Mana)
	// Called when Mana changes on clients to propagate GAS notifications.
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana) const;

	/*
	* Meta Attributes
	* Transient inputs used by effects (not typically replicated to UI).
	*/

	// Damage received from a GameplayEffect before being applied to Health.
	// This is consumed in PostGameplayEffectExecute and then reset to 0.
	UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UAuraAttributeSet, IncomingDamage);
};
