// Portfolio excerpt from UEquipmentComponent.
// Helper implementations unrelated to the GAS lifecycle were omitted.

bool UEquipmentComponent::ApplyWeaponEquipmentState(
	const FItemInstance& Item,
	const UWeaponDefinitionAsset* Definition)
{
	if (Definition == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetASC();
	if (ASC == nullptr)
	{
		return false;
	}

	const int32 AbilityLevel = FMath::Max(1, Item.Level);
	const float EffectLevel = static_cast<float>(AbilityLevel);

	// Runtime data belongs to this specific item instance. It records the
	// handles required to reverse every gameplay change during unequip.
	FEquippedItemRuntimeData& RuntimeData =
		EquippedRuntimeMap.FindOrAdd(Item.InstanceID);
	RuntimeData.SourceInstanceID = Item.InstanceID;
	RuntimeData.CachedWeaponType = Definition->WeaponType;

	for (const FWeaponAbilityEntry& AbilityEntry : Definition->GrantedAbilities)
	{
		if (AbilityEntry.AbilityClass == nullptr)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityEntry.AbilityClass, AbilityLevel);

		// Dynamic source tags connect a data-defined weapon ability with the
		// input-routing and attack-type systems without hard-coding the class.
		if (AbilityEntry.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityEntry.InputTag);
		}
		if (AbilityEntry.AttackTypeTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityEntry.AttackTypeTag);
		}

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		RuntimeData.GrantedAbilityHandles.Add(Handle);
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass :
		 Definition->PassiveEffectsWhileEquipped)
	{
		if (EffectClass == nullptr)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FActiveGameplayEffectHandle EffectHandle =
			ASC->BP_ApplyGameplayEffectToSelf(
				EffectClass,
				EffectLevel,
				EffectContext);
		RuntimeData.ActiveEffectHandles.Add(EffectHandle);
	}

	// Visual setup is part of the same transaction. If it fails, the method
	// rolls back the already granted abilities and effects.
	if (!SpawnEquippedActor(Item, Definition))
	{
		RemoveWeaponEquipmentState(Item);
		return false;
	}

	if (ADyplomPlayerCharacter* PlayerCharacter =
		Cast<ADyplomPlayerCharacter>(GetOwningCharacter()))
	{
		PlayerCharacter->SetWeaponCarryStateFromEquipment(
			EWeaponCarryState::InHand);
	}
	else
	{
		RefreshCurrentWeaponAttachment(EWeaponCarryState::InHand);
	}

	return true;
}

bool UEquipmentComponent::RemoveWeaponEquipmentState(const FItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetASC();
	FEquippedItemRuntimeData* RuntimeData =
		EquippedRuntimeMap.Find(Item.InstanceID);

	// Missing runtime state is treated as already cleaned up, making repeated
	// removal requests safe.
	if (RuntimeData == nullptr)
	{
		return true;
	}

	if (ASC != nullptr)
	{
		for (const FGameplayAbilitySpecHandle& Handle :
			 RuntimeData->GrantedAbilityHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}

		for (const FActiveGameplayEffectHandle& EffectHandle :
			 RuntimeData->ActiveEffectHandles)
		{
			if (EffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
	}

	DestroyEquippedActor(Item.InstanceID);
	EquippedRuntimeMap.Remove(Item.InstanceID);
	return true;
}

