// Portfolio excerpt from InventoryTypes.h.
// UI-facing data and unrelated inventory operations were omitted.

#pragma once

#include "Net/Serialization/FastArraySerializer.h"

class UInventoryComponent;
struct FInventoryList;

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	FItemInstance Item;

	// Fast Array callbacks notify the owning component after replicated
	// additions, changes, or removals reach a client.
	void PreReplicatedRemove(const FInventoryList& InArraySerializer);
	void PostReplicatedAdd(const FInventoryList& InArraySerializer);
	void PostReplicatedChange(const FInventoryList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FInventoryList() = default;

	explicit FInventoryList(UInventoryComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	UInventoryComponent* OwnerComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		// Fast Array delta serialization sends only entries changed since the
		// previous replication update instead of resending the entire inventory.
		return FFastArraySerializer::FastArrayDeltaSerialize<
			FInventoryEntry,
			FInventoryList>(Entries, DeltaParms, *this);
	}

	FInventoryEntry* FindEntryBySlot(int32 SlotIndex)
	{
		return Entries.FindByPredicate(
			[SlotIndex](const FInventoryEntry& Entry)
			{
				return Entry.SlotIndex == SlotIndex;
			});
	}

	FInventoryEntry* FindEntryByInstanceID(const FGuid& InstanceID)
	{
		return Entries.FindByPredicate(
			[&InstanceID](const FInventoryEntry& Entry)
			{
				return Entry.Item.InstanceID == InstanceID;
			});
	}

	bool AddItemToSlot(const FItemInstance& Item, int32 SlotIndex)
	{
		if (FindEntryBySlot(SlotIndex) != nullptr)
		{
			return false;
		}

		FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.SlotIndex = SlotIndex;
		NewEntry.Item = Item;

		// Marking the individual entry allows Fast Array replication to send
		// only this addition.
		MarkItemDirty(NewEntry);
		return true;
	}

	bool MoveItem(int32 FromSlot, int32 ToSlot)
	{
		FInventoryEntry* FromEntry = FindEntryBySlot(FromSlot);
		if (FromEntry == nullptr || FindEntryBySlot(ToSlot) != nullptr)
		{
			return false;
		}

		FromEntry->SlotIndex = ToSlot;
		MarkItemDirty(*FromEntry);
		return true;
	}

	bool SwapItems(int32 FirstSlot, int32 SecondSlot)
	{
		FInventoryEntry* FirstEntry = FindEntryBySlot(FirstSlot);
		FInventoryEntry* SecondEntry = FindEntryBySlot(SecondSlot);
		if (FirstEntry == nullptr || SecondEntry == nullptr)
		{
			return false;
		}

		Swap(FirstEntry->SlotIndex, SecondEntry->SlotIndex);
		MarkItemDirty(*FirstEntry);
		MarkItemDirty(*SecondEntry);
		return true;
	}
};

template <>
struct TStructOpsTypeTraits<FInventoryList>
	: public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
