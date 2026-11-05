// Portfolio excerpt from UQuickWheelComponent.
// UI-specific view-data code was omitted to highlight item identity,
// authoritative activation, and inventory/equipment integration.

enum class EQuickWheelReferenceType : uint8
{
	ItemID,
	InstanceID
};

struct FQuickWheelEntry
{
	EQuickWheelReferenceType ReferenceType = EQuickWheelReferenceType::ItemID;
	FName ItemID = NAME_None;
	FGameplayTag Rarity;
	FGuid InstanceID;

	bool operator==(const FQuickWheelEntry& Other) const
	{
		return ReferenceType == Other.ReferenceType &&
			(ReferenceType == EQuickWheelReferenceType::ItemID
				? ItemID == Other.ItemID && Rarity == Other.Rarity
				: InstanceID == Other.InstanceID);
	}
};

void UQuickWheelComponent::RequestActivateFavorite(int32 FavoriteIndex)
{
	if (GetOwner() != nullptr && GetOwner()->HasAuthority())
	{
		ActivateFavoriteInternal(FavoriteIndex);
		return;
	}

	// The client only requests an action. Item resolution and the resulting
	// inventory/equipment mutation are performed on the authority.
	ServerRequestActivateFavorite(FavoriteIndex);
}

void UQuickWheelComponent::ServerRequestActivateFavorite_Implementation(
	int32 FavoriteIndex)
{
	ActivateFavoriteInternal(FavoriteIndex);
}

bool UQuickWheelComponent::BuildReferenceForItem(
	const FGuid& InstanceID,
	FQuickWheelEntry& OutReference) const
{
	const FItemInstance* Item = nullptr;
	bool bEquipped = false;
	FGameplayTag SlotTag;
	if (!FindItemInstance(InstanceID, Item, bEquipped, SlotTag) || Item == nullptr)
	{
		return false;
	}

	const UInventoryComponent* Inventory = GetInventoryComponent();
	const UItemDefinitionAsset* Definition =
		Inventory != nullptr ? Inventory->GetItemDefinition(Item->ItemID) : nullptr;
	if (Definition == nullptr ||
		(!Definition->IsConsumable() && !Definition->IsWeapon()))
	{
		return false;
	}

	OutReference = FQuickWheelEntry();
	OutReference.ItemID = Item->ItemID;
	OutReference.Rarity = Item->Rarity;

	// A favorite potion should survive consuming one stack and finding another
	// stack of the same variant. Unique gear must keep pointing at one instance.
	if (Definition->IsConsumable())
	{
		OutReference.ReferenceType = EQuickWheelReferenceType::ItemID;
	}
	else
	{
		OutReference.ReferenceType = EQuickWheelReferenceType::InstanceID;
		OutReference.InstanceID = Item->InstanceID;
	}

	return true;
}

void UQuickWheelComponent::ActivateFavoriteInternal(int32 FavoriteIndex)
{
	if (!Favorites.IsValidIndex(FavoriteIndex))
	{
		return;
	}

	const FItemInstance* Item = nullptr;
	bool bEquipped = false;
	FGameplayTag EquipmentSlot;
	if (!ResolveReference(
		Favorites[FavoriteIndex], Item, bEquipped, EquipmentSlot) || Item == nullptr)
	{
		return;
	}

	UInventoryComponent* Inventory = GetInventoryComponent();
	UEquipmentComponent* Equipment = GetEquipmentComponent();
	const UItemDefinitionAsset* Definition =
		Inventory != nullptr ? Inventory->GetItemDefinition(Item->ItemID) : nullptr;
	if (Definition == nullptr)
	{
		return;
	}

	// One radial-menu action adapts to the resolved item's current location.
	if (Definition->IsConsumable())
	{
		if (bEquipped && Equipment != nullptr)
		{
			Equipment->RequestUseEquippedConsumableSlot(EquipmentSlot);
		}
		else if (Inventory != nullptr)
		{
			Inventory->RequestUseConsumable(Item->InstanceID);
		}
	}
	else if (Definition->IsWeapon() && !bEquipped && Inventory != nullptr)
	{
		Inventory->RequestEquipItem(Item->InstanceID);
	}
}
