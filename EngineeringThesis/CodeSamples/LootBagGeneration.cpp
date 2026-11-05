// Portfolio excerpt from UDyplomLootSubsystem.
// Data-table resolution and validation were shortened to focus on the shared
// category bag, weighted selection, and server-authoritative item generation.

void RefillBag(
	FDyplomLootBagRuntimeState& BagState,
	const TMap<FGameplayTag, int32>& ConfiguredTokens)
{
	BagState.RemainingTokens = ConfiguredTokens;
	BagState.TotalRemainingTokens = 0;

	for (const TPair<FGameplayTag, int32>& Pair : BagState.RemainingTokens)
	{
		BagState.TotalRemainingTokens += FMath::Max(0, Pair.Value);
	}
}

bool UDyplomLootSubsystem::GenerateLoot(
	const ULootTableDataAsset* Profile,
	UDataTable* ItemDataTable,
	int32 LootSourceLevel,
	TArray<FItemInstance>& OutItems)
{
	OutItems.Reset();

	// Clients never roll rewards locally. The generated item instances are later
	// stored by the authoritative chest component and replicated to viewers.
	const UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client ||
		Profile == nullptr || ItemDataTable == nullptr)
	{
		return false;
	}

	const FName BagID = ResolveProfileID(Profile);
	TMap<FGameplayTag, int32> CategoryTokens;
	for (const FLootCategoryBagEntry& BagEntry : Profile->CategoryBag)
	{
		if (BagEntry.ItemCategory.IsValid() && BagEntry.TokenCount > 0)
		{
			CategoryTokens.FindOrAdd(BagEntry.ItemCategory) += BagEntry.TokenCount;
		}
	}

	// Resolving the profile's Data Table rows and item definitions is omitted.
	// Each entry below already contains its loot rule and item template.
	TArray<FResolvedLootEntry> Entries;
	// ...populate Entries from Profile->LootEntryDataTable and ItemDataTable...
	if (CategoryTokens.IsEmpty() || Entries.IsEmpty())
	{
		return false;
	}

	// Every chest using the same LootTableID reaches the same runtime bag.
	// This turns pure randomness into a finite draw-without-replacement cycle.
	FDyplomLootBagRuntimeState& Bag = LootBags.FindOrAdd(BagID);
	if (!Bag.bInitialized)
	{
		Bag.RandomStream.Initialize(
			static_cast<int32>(FPlatformTime::Cycles64() ^ GetTypeHash(BagID)));
		Bag.bInitialized = true;
		RefillBag(Bag, CategoryTokens);
	}

	const int32 MinimumRolls = FMath::Max(1, Profile->MinLootRolls);
	const int32 MaximumRolls = FMath::Max(MinimumRolls, Profile->MaxLootRolls);
	const int32 RollCount = Bag.RandomStream.RandRange(MinimumRolls, MaximumRolls);

	for (int32 RollIndex = 0; RollIndex < RollCount; ++RollIndex)
	{
		if (Bag.TotalRemainingTokens <= 0)
		{
			RefillBag(Bag, CategoryTokens);
		}

		TMap<FGameplayTag, TArray<FResolvedLootEntry>> EligibleByCategory;
		for (const TPair<FGameplayTag, int32>& Remaining : Bag.RemainingTokens)
		{
			if (Remaining.Value <= 0)
			{
				continue;
			}

			TArray<FResolvedLootEntry>& CategoryEntries =
				EligibleByCategory.FindOrAdd(Remaining.Key);
			for (const FResolvedLootEntry& Entry : Entries)
			{
				const bool bMatchesLevel =
					LootSourceLevel >= Entry.LootEntry->MinWorldLevel &&
					LootSourceLevel <= Entry.LootEntry->MaxWorldLevel;
				const bool bMatchesCategory =
					DoesItemMatchCategory(*Entry.ItemTemplate, Remaining.Key);
				const bool bHasRarity = Profile->RarityWeights.ContainsByPredicate(
					[&Entry](const FLootRarityWeight& Rarity)
					{
						return Rarity.Weight > 0 &&
							DoesEntryAllowRarity(*Entry.LootEntry, Rarity.RarityTag);
					});

				if (bMatchesLevel && bMatchesCategory && bHasRarity)
				{
					CategoryEntries.Add(Entry);
				}
			}

			if (CategoryEntries.IsEmpty())
			{
				EligibleByCategory.Remove(Remaining.Key);
			}
		}

		TArray<FGameplayTag> EligibleCategories;
		EligibleByCategory.GetKeys(EligibleCategories);
		int32 EligibleTokenCount = 0;
		for (const FGameplayTag& Candidate : EligibleCategories)
		{
			EligibleTokenCount += Bag.RemainingTokens.FindRef(Candidate);
		}
		if (EligibleTokenCount <= 0)
		{
			break;
		}

		const int32 CategoryRoll = Bag.RandomStream.RandRange(1, EligibleTokenCount);
		int32 RunningTokens = 0;
		FGameplayTag Category;
		for (const FGameplayTag& Candidate : EligibleCategories)
		{
			RunningTokens += Bag.RemainingTokens.FindRef(Candidate);
			if (CategoryRoll <= RunningTokens)
			{
				Category = Candidate;
				break;
			}
		}

		const TArray<FResolvedLootEntry>* CategoryEntries =
			EligibleByCategory.Find(Category);
		if (CategoryEntries == nullptr)
		{
			break;
		}

		// Selection is hierarchical: category -> rarity -> weighted item ->
		// quantity/level/enchantments -> unique item instance.
		const FLootRarityWeight* Rarity = SelectRarity(
			*Profile, *CategoryEntries, Bag.RandomStream);
		const FResolvedLootEntry* Entry = Rarity != nullptr
			? SelectItemEntry(*CategoryEntries, Rarity->RarityTag, Bag.RandomStream)
			: nullptr;

		FItemInstance GeneratedItem;
		if (Entry == nullptr || !BuildItemInstance(
				*Profile,
				*Entry,
				FMath::Max(1, LootSourceLevel),
				Rarity->RarityTag,
				Bag.RandomStream,
				GeneratedItem))
		{
			break;
		}

		int32& Tokens = Bag.RemainingTokens.FindChecked(Category);
		Tokens = FMath::Max(0, Tokens - 1);
		Bag.TotalRemainingTokens = FMath::Max(0, Bag.TotalRemainingTokens - 1);
		OutItems.Add(MoveTemp(GeneratedItem));
	}

	return !OutItems.IsEmpty();
}
