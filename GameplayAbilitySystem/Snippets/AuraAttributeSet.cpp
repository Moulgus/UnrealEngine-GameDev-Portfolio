void UAuraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Handle damage routed through the "IncomingDamage" attribute.
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		// Reset the transient damage attribute so it doesn't accumulate across frames.
		SetIncomingDamage(0.f);

		if (LocalIncomingDamage > 0.f)
		{
			// Apply damage to Health and clamp the result within valid bounds.
			const float NewHealth = GetHealth() - LocalIncomingDamage;
			SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
			
			// Determine whether the damage is fatal.
			const bool bFatal = NewHealth <= 0.f;

			if (bFatal)
			{
				// Notify the target via its combat interface to handle death (ragdoll, animations, etc.).
				ICombatInterface* CombatInterface = Cast<ICombatInterface>(Properties.TargetAvatarActor);
				if (CombatInterface)
				{
					CombatInterface->Die();
				}
			}
			else
			{
				// Trigger a hit reaction ability on the target if not dead.
				FGameplayTagContainer TagContainer;
				TagContainer.AddTag(FAuraGameplayTags::Get().Effects_HitReact);
				Properties.TargetASC->TryActivateAbilitiesByTag(TagContainer);
			}

			// Read contextual flags produced by the effect (e.g., block/crit) to enrich feedback.
			const bool bBlock = UAuraAbilitySystemLibrary::IsBlockedHit(Properties.EffectContextHandle);
			const bool bCritical = UAuraAbilitySystemLibrary::IsCriticalHit(Properties.EffectContextHandle);

			// Show floating combat text to the source player (damage numbers, block/crit indicators).
			ShowFloatingText(Properties, LocalIncomingDamage, bBlock, bCritical);
		}
	}
}

// Sends floating damage numbers to the source player's HUD.
// Only shown if the source is different from the target to avoid self-damage spam.
void UAuraAttributeSet::ShowFloatingText(const FEffectProperties& Properties, float Damage, bool bBlockedHit, bool bCriticalHit) const
{
	if (Properties.SourceCharacter != Properties.TargetCharacter)
	{
		// Route UI feedback through the source player's controller.
		if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Properties.SourceCharacter->Controller))
		{
			PC->ShowDamageNumber(Damage, Properties.TargetCharacter, bBlockedHit, bCriticalHit);
		}
	}
}

// -----------------------------
// Replication callbacks (Vital)
// -----------------------------

// Rep-notify for Health attribute. Ensures clients update UI/logic when health changes.
void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health, OldHealth);
}

// Rep-notify for Mana attribute. Ensures clients update UI/logic when mana changes.
void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana, OldMana);
}
