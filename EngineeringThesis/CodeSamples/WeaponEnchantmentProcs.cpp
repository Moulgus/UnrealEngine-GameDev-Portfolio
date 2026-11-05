// Portfolio excerpts from UWeaponEnchantDefinitionAsset and
// UDyplomGameplayAbility_Combat. Repeated validation, spatial-query details,
// and VFX spawning were omitted. The shown logic comes from the project.

UENUM(BlueprintType)
enum class EWeaponEnchantProcType : uint8
{
	None,
	GameplayEffectOnly,
	PeriodicDamage,
	StackThreshold,
	ChainDamage
};

UENUM(BlueprintType)
enum class EWeaponEnchantEffectLevelSource : uint8
{
	WeaponLevel,
	EnchantLevel
};

UCLASS(BlueprintType)
class DYPLOM_API UWeaponEnchantDefinitionAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// The same ability code handles all enchant variants. Designers select the
	// proc model and tune its scalable values in a Data Asset.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc")
	EWeaponEnchantProcType ProcType = EWeaponEnchantProcType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc")
	FGameplayTag ProcBlockedByTargetTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Effects")
	TSubclassOf<UGameplayEffect> OnHitEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Effects")
	EWeaponEnchantEffectLevelSource OnHitEffectLevelSource =
		EWeaponEnchantEffectLevelSource::WeaponLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Damage")
	FScalableFloat ProcDamageRatio;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Damage")
	FGameplayTag ProcDamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Periodic")
	int32 PeriodicTickCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Periodic")
	bool bReplaceExistingOnHitEffect = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Threshold")
	int32 StackThreshold = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Threshold")
	TSubclassOf<UGameplayEffect> ThresholdEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Threshold")
	EWeaponEnchantEffectLevelSource ThresholdEffectLevelSource =
		EWeaponEnchantEffectLevelSource::EnchantLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Chain")
	float ChainRadius = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Chain")
	int32 ChainMaxTargets = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enchant|Proc|Chain")
	float ChainDamageDecay = 0.75f;

	float GetProcDamageRatioForLevel(int32 EnchantLevel) const;
};

void UDyplomGameplayAbility_Combat::ApplyWeaponEnchantOnHitEffectsToTarget(
	AActor* HitActor,
	const FComputedDamagePayload& SourceDamagePayload,
	const FHitResult* HitResult) const
{
	if (HitActor == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (TargetASC == nullptr)
	{
		return;
	}

	const FDyplomGameplayTags& Tags = FDyplomGameplayTags::Get();
	if (TargetASC->HasMatchingGameplayTag(Tags.State_Defense_Invulnerable) ||
		TargetASC->HasMatchingGameplayTag(Tags.State_Dead))
	{
		return;
	}

	const UWeaponDefinitionAsset* WeaponDefinition = GetCurrentWeaponDefinition();
	const FItemInstance* WeaponItemInstance = GetCurrentWeaponItemInstance();
	if (WeaponDefinition == nullptr || WeaponItemInstance == nullptr)
	{
		return;
	}

	const TArray<FWeaponEnchantEntry> ResolvedEnchantEntries =
		DyplomWeaponDamageHelper::GetResolvedWeaponEnchantEntries(
			WeaponDefinition, *WeaponItemInstance);
	const float SourcePreResistanceDamage =
		SourceDamagePayload.GetTotalMagnitude();
	auto ResolveEffectLevel = [&](const FWeaponEnchantEntry& Entry,
		EWeaponEnchantEffectLevelSource LevelSource)
	{
		return LevelSource == EWeaponEnchantEffectLevelSource::WeaponLevel
			? FMath::Max(1, WeaponItemInstance->Level)
			: FMath::Max(1, Entry.Level);
	};

	for (const FWeaponEnchantEntry& EnchantEntry : ResolvedEnchantEntries)
	{
		UWeaponEnchantDefinitionAsset* Definition =
			EnchantEntry.EnchantDefinition;
		if (Definition == nullptr || Definition->OnHitEffectClass == nullptr)
		{
			continue;
		}

		// Gameplay Tags provide a generic immunity/cooldown gate without
		// coupling the combat ability to a specific enchant or enemy class.
		if (Definition->ProcBlockedByTargetTag.IsValid() &&
			TargetASC->HasMatchingGameplayTag(Definition->ProcBlockedByTargetTag))
		{
			continue;
		}

		const FGameplayTag ProcDamageType =
			Definition->ProcDamageTypeTag.IsValid()
				? Definition->ProcDamageTypeTag
				: Definition->DamageTypeTag;
		const float ProcDamageRatio =
			Definition->GetProcDamageRatioForLevel(EnchantEntry.Level);

		if (Definition->ProcType == EWeaponEnchantProcType::ChainDamage)
		{
			// The full implementation repeatedly searches around the previous
			// target, excludes actors already hit, and selects the nearest valid
			// target. Damage falls off after every jump.
			for (int32 ChainIndex = 0;
				ChainIndex < Definition->ChainMaxTargets;
				++ChainIndex)
			{
				const float ChainMagnitude = SourcePreResistanceDamage
					* ProcDamageRatio
					* FMath::Pow(
						FMath::Clamp(Definition->ChainDamageDecay, 0.0f, 1.0f),
						ChainIndex);

				// ChainMagnitude is passed to the next target through a
				// SetByCaller damage tag. Replicated spline VFX connects targets.
			}
			continue;
		}

		if (Definition->ProcType == EWeaponEnchantProcType::PeriodicDamage &&
			Definition->bReplaceExistingOnHitEffect)
		{
			FGameplayEffectQuery ExistingEffectQuery;
			ExistingEffectQuery.EffectDefinition = Definition->OnHitEffectClass;
			TargetASC->RemoveActiveEffects(ExistingEffectQuery);
		}

		const int32 EffectLevel = ResolveEffectLevel(
			EnchantEntry, Definition->OnHitEffectLevelSource);
		FGameplayEffectSpecHandle EffectSpec = MakeOutgoingGameplayEffectSpec(
			Definition->OnHitEffectClass,
			static_cast<float>(EffectLevel));
		if (!EffectSpec.IsValid() || !EffectSpec.Data.IsValid())
		{
			continue;
		}

		if (Definition->ProcType == EWeaponEnchantProcType::PeriodicDamage &&
			SourcePreResistanceDamage > 0.0f && ProcDamageRatio > 0.0f &&
			ProcDamageType.IsValid())
		{
			const int32 TickCount = FMath::Max(1, Definition->PeriodicTickCount);
			const float DamagePerTick = SourcePreResistanceDamage
				* ProcDamageRatio / static_cast<float>(TickCount);
			EffectSpec.Data->SetSetByCallerMagnitude(
				ProcDamageType, DamagePerTick);
			EffectSpec.Data->AddDynamicAssetTag(Tags.Damage_Periodic);
		}

		const FActiveGameplayEffectHandle AppliedHandle =
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (ADyplomCharacterBase* Target =
			Cast<ADyplomCharacterBase>(HitActor))
		{
			// Stack progress is exposed to the replicated buildup UI.
			Target->TrackEnchantBuildup(Definition, AppliedHandle);
		}

		if (Definition->ProcType != EWeaponEnchantProcType::StackThreshold ||
			Definition->ThresholdEffectClass == nullptr)
		{
			continue;
		}

		FGameplayEffectQuery StackQuery;
		StackQuery.EffectDefinition = Definition->OnHitEffectClass;
		const int32 CurrentStacks =
			TargetASC->GetAggregatedStackCount(StackQuery);
		if (CurrentStacks < FMath::Max(1, Definition->StackThreshold))
		{
			continue;
		}

		// Reaching the configured threshold consumes buildup before applying
		// the payoff effect (for example Frostbite or Wind Stagger).
		TargetASC->RemoveActiveEffects(StackQuery);
		const int32 ThresholdEffectLevel = ResolveEffectLevel(
			EnchantEntry, Definition->ThresholdEffectLevelSource);
		FGameplayEffectSpecHandle ThresholdSpec =
			MakeOutgoingGameplayEffectSpec(
				Definition->ThresholdEffectClass,
				static_cast<float>(ThresholdEffectLevel));
		if (ThresholdSpec.IsValid() && ThresholdSpec.Data.IsValid())
		{
			if (SourcePreResistanceDamage > 0.0f && ProcDamageRatio > 0.0f &&
				ProcDamageType.IsValid())
			{
				ThresholdSpec.Data->SetSetByCallerMagnitude(
					ProcDamageType,
					SourcePreResistanceDamage * ProcDamageRatio);
			}
			if (HitResult != nullptr)
			{
				ThresholdSpec.Data->GetContext().AddHitResult(*HitResult, true);
			}
			TargetASC->ApplyGameplayEffectSpecToSelf(
				*ThresholdSpec.Data.Get());
		}
	}
}
