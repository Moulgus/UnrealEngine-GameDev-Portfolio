// Portfolio excerpt from UInventoryComponent.
// The UI request wrappers and unrelated inventory operations were omitted.

void UInventoryComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, InventoryList);
}

int32 UInventoryComponent::TransferStackAmount(
	const FGuid& SourceInstanceID,
	int32 Amount,
	int32 TargetSlot)
{
	// Inventory mutation is accepted only by the authority. The same validation
	// protects calls coming from gameplay code and from server RPC wrappers.
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0 ||
		TargetSlot < 0 || TargetSlot >= MaxInventorySlots)
	{
		return 0;
	}

	FInventoryEntry* SourceEntry =
		InventoryList.FindEntryByInstanceID(SourceInstanceID);
	if (SourceEntry == nullptr ||
		SourceEntry->SlotIndex == TargetSlot ||
		SourceEntry->Item.Quantity <= 0)
	{
		return 0;
	}

	const int32 SourceSlot = SourceEntry->SlotIndex;
	const int32 SourceQuantity = SourceEntry->Item.Quantity;
	const int32 SafeAmount = FMath::Min(Amount, SourceQuantity);
	FInventoryEntry* TargetEntry = InventoryList.FindEntryBySlot(TargetSlot);

	if (TargetEntry == nullptr)
	{
		// Moving the entire quantity preserves the original item instance.
		if (SafeAmount == SourceQuantity)
		{
			return MoveItem(SourceSlot, TargetSlot) ? SafeAmount : 0;
		}

		// A partial split creates a new runtime item instance with a unique ID.
		FItemInstance NewStack = SourceEntry->Item;
		NewStack.InstanceID = FGuid::NewGuid();
		NewStack.Quantity = SafeAmount;

		if (!InventoryList.AddItemToSlot(NewStack, TargetSlot))
		{
			return 0;
		}

		// Re-resolve after modifying the array to avoid using an invalidated
		// pointer if its storage was reallocated.
		SourceEntry = InventoryList.FindEntryByInstanceID(SourceInstanceID);
		if (SourceEntry == nullptr)
		{
			InventoryList.RemoveItemByInstanceID(NewStack.InstanceID);
			return 0;
		}

		SourceEntry->Item.Quantity -= SafeAmount;
		InventoryList.MarkItemDirty(*SourceEntry);
		BroadcastInventoryChanged();
		return SafeAmount;
	}

	int32 MaxStack = 1;
	if (!IsItemStackable(SourceEntry->Item, MaxStack) ||
		!CanItemsStackTogether(SourceEntry->Item, TargetEntry->Item))
	{
		return 0;
	}

	const int32 SpaceLeft = MaxStack - TargetEntry->Item.Quantity;
	const int32 TransferredAmount =
		FMath::Min(SafeAmount, FMath::Max(0, SpaceLeft));
	if (TransferredAmount <= 0)
	{
		return 0;
	}

	TargetEntry->Item.Quantity += TransferredAmount;
	SourceEntry->Item.Quantity -= TransferredAmount;
	InventoryList.MarkItemDirty(*TargetEntry);

	if (SourceEntry->Item.Quantity <= 0)
	{
		InventoryList.RemoveItemByInstanceID(SourceInstanceID);
	}
	else
	{
		InventoryList.MarkItemDirty(*SourceEntry);
	}

	BroadcastInventoryChanged();
	return TransferredAmount;
}

void UInventoryComponent::Server_RequestTransferStackAmount_Implementation(
	const FGuid& SourceInstanceID,
	int32 Amount,
	int32 TargetSlot,
	int32 RequestId)
{
	const int32 TransferredAmount =
		TransferStackAmount(SourceInstanceID, Amount, TargetSlot);

	// The UI receives the amount confirmed by the server, which may be lower
	// than requested when the target stack has limited capacity.
	Client_ReceiveStackTransferResult(RequestId, TransferredAmount);
}

void UInventoryComponent::Client_ReceiveStackTransferResult_Implementation(
	int32 RequestId,
	int32 TransferredAmount)
{
	OnStackTransferResult.Broadcast(RequestId, TransferredAmount);
}

