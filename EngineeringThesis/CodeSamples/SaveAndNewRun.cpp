// Portfolio excerpt from UDyplomSaveGameSubsystem.
// UI, thumbnail, and error-reporting code was omitted to highlight the
// difference between loading a save and carrying progression into a new run.

bool UDyplomSaveGameSubsystem::BeginNewRun()
{
	if (!CanSaveCurrentGame() || PendingLoadedSave != nullptr)
	{
		return false;
	}

	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	ADyplomPlayerState* PlayerState = Controller
		? Controller->GetPlayerState<ADyplomPlayerState>()
		: nullptr;
	UWorld* World = GetWorld();
	if (PlayerState == nullptr || !PlayerState->HasAuthority() || World == nullptr)
	{
		return false;
	}

	// Capture player progression only. The previous run's doors, chests and
	// cleared encounters are deliberately not restored on the next map load.
	UDyplomSaveGame* Transition = CaptureCurrentPlayer(
		TEXT("Dyplom_RunTransition"),
		TEXT("Run Transition"),
		EDyplomSaveSlotType::Auto,
		false);
	if (Transition == nullptr)
	{
		return false;
	}

	PlayerState->IncrementCompletedRunCount();
	Transition->CompletedRunCount = PlayerState->GetCompletedRunCount();
	Transition->bStartNewRunTransition = true;
	PendingLoadedSave = Transition;
	bPendingProgressionPrepared = false;

	const FString CurrentMap =
		UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	UGameplayStatics::OpenLevel(this, FName(*CurrentMap), true);
	return true;
}

bool UDyplomSaveGameSubsystem::ApplyPendingSaveToPlayer(
	ADyplomPlayerCharacter* PlayerCharacter)
{
	if (PendingLoadedSave == nullptr || !bPendingProgressionPrepared ||
		PlayerCharacter == nullptr || !PlayerCharacter->HasAuthority())
	{
		return false;
	}

	ADyplomPlayerState* PlayerState =
		PlayerCharacter->GetPlayerState<ADyplomPlayerState>();
	UInventoryComponent* Inventory =
		PlayerState != nullptr ? PlayerState->GetInventoryComponent() : nullptr;
	UEquipmentComponent* Equipment =
		PlayerState != nullptr ? PlayerState->GetEquipmentComponent() : nullptr;
	UQuickWheelComponent* QuickWheel =
		PlayerState != nullptr ? PlayerState->GetQuickWheelComponent() : nullptr;
	UDyplomAttributeSet* Attributes = PlayerState != nullptr
		? Cast<UDyplomAttributeSet>(PlayerState->GetAttributeSet())
		: nullptr;
	UAbilitySystemComponent* ASC =
		PlayerState != nullptr ? PlayerState->GetAbilitySystemComponent() : nullptr;
	if (PlayerState == nullptr || Inventory == nullptr || Equipment == nullptr ||
		QuickWheel == nullptr || Attributes == nullptr || ASC == nullptr)
	{
		return false;
	}

	const bool bNewRun = PendingLoadedSave->bStartNewRunTransition;
	PlayerState->ApplyRestoredVitalInvestments();
	const bool bItemsRestored =
		Inventory->RestoreInventoryFromSave(PendingLoadedSave->InventoryEntries) &&
		Equipment->RestoreEquipmentFromSave(PendingLoadedSave->EquipmentEntries) &&
		QuickWheel->RestoreFavoritesFromSave(PendingLoadedSave->QuickWheelEntries);

	// A regular load reconstructs the saved world. A new run keeps the freshly
	// loaded map and restores only persistent player state.
	const bool bWorldRestored = bNewRun ||
		ApplyWorldState(PendingLoadedSave->WorldState);

	const float MaxHealth = Attributes->GetMaxHealth();
	const float MaxMana = Attributes->GetMaxMana();
	const float MaxStamina = Attributes->GetMaxStamina();
	ASC->SetNumericAttributeBase(
		UDyplomAttributeSet::GetHealthAttribute(),
		bNewRun ? MaxHealth : FMath::Clamp(PendingLoadedSave->Health, 1.0f, MaxHealth));
	ASC->SetNumericAttributeBase(
		UDyplomAttributeSet::GetManaAttribute(),
		bNewRun ? MaxMana : FMath::Clamp(PendingLoadedSave->Mana, 0.0f, MaxMana));
	ASC->SetNumericAttributeBase(
		UDyplomAttributeSet::GetStaminaAttribute(),
		bNewRun ? MaxStamina : FMath::Clamp(PendingLoadedSave->Stamina, 0.0f, MaxStamina));

	if (!bNewRun)
	{
		PlayerCharacter->SetActorTransform(
			PendingLoadedSave->PlayerTransform,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	PendingLoadedSave = nullptr;
	bPendingProgressionPrepared = false;
	return bItemsRestored && bWorldRestored;
}

void UDyplomSaveGameSubsystem::CaptureCurrentWorld(
	FDyplomWorldSaveState& OutState) const
{
	OutState = FDyplomWorldSaveState();
	OutState.bHasWorldState = true;

	// Stable authored IDs allow state to be matched after actors are recreated.
	for (TActorIterator<ADyplomWaveArena> It(GetWorld()); It; ++It)
	{
		if (It->IsCompleted())
		{
			OutState.CompletedArenaIds.Add(It->GetArenaId());
		}
	}

	for (TActorIterator<ALootChest> It(GetWorld()); It; ++It)
	{
		if (!It->ShouldPersistState())
		{
			continue;
		}

		FDyplomSavedChestState& Chest = OutState.ChestStates.AddDefaulted_GetRef();
		Chest.ChestId = It->GetChestId();
		Chest.bLootGenerated = It->HasGeneratedLoot();
		if (const UChestInventoryComponent* Inventory =
			It->GetChestInventoryComponent())
		{
			for (const FChestInventoryEntry& Entry : Inventory->GetChestInventory().Entries)
			{
				FDyplomSavedChestEntry& SavedEntry =
					Chest.Entries.AddDefaulted_GetRef();
				SavedEntry.SlotIndex = Entry.SlotIndex;
				SavedEntry.Item = Entry.Item;
			}
		}
	}

	for (TActorIterator<ADyplomDoorBase> It(GetWorld()); It; ++It)
	{
		if (!It->ShouldPersistState())
		{
			continue;
		}

		FDyplomSavedDoorState& Door = OutState.DoorStates.AddDefaulted_GetRef();
		Door.DoorId = It->GetDoorId();
		Door.bIsOpen = It->IsDoorOpen();
		Door.bLocked = It->IsDoorLocked();
		Door.OpeningDirection = It->GetOpeningDirection();
	}
}
