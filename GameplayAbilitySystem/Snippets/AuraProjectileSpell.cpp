

void UAuraProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	UE_LOG(LogTemp, Warning, TEXT("UAuraProjectileSpell::SpawnProjectile on %s"), *GetNameSafe(GetAvatarActorFromActorInfo()));

	// Authority check: only the server should spawn gameplay-affecting actors.
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer)
	{
		return;
	}

	// Access combat-specific data (e.g., socket location) from the avatar.
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo());

	if (CombatInterface)
	{
		// Determine spawn origin and forward rotation towards the target location.
		const FVector SocketLocation = CombatInterface->GetCombatSocketLocation();
		FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
		// Rotation.Pitch can be constrained if you want planar trajectories.

		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SocketLocation);
		SpawnTransform.SetRotation(Rotation.Quaternion());

		// Deferred spawn to allow initialization before BeginPlay.
		AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			Cast<APawn>(GetOwningActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		// Build an effect context that carries ability, source object, involved actors, and an initial hit location.
		const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());

		FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
		EffectContextHandle.SetAbility(this);
		EffectContextHandle.AddSourceObject(Projectile);
		TArray<TWeakObjectPtr<AActor>> Actors;
		Actors.Add(Projectile);
		EffectContextHandle.AddActors(Actors);
		FHitResult HitResult;
		HitResult.Location = ProjectileTargetLocation;
		EffectContextHandle.AddHitResult(HitResult);

		// Create the outgoing damage spec seeded with the context and current ability level.
		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContextHandle);

		// Assign per-type damage using SetByCaller magnitudes so ExecCalc_Damage can aggregate them.
		const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
		for (auto& DamageTypePair : DamageTypes)
		{
			const float ScaledDamage = DamageTypePair.Value.GetValueAtLevel(GetAbilityLevel());
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageTypePair.Key, ScaledDamage);
		}

		// Pass the configured spec to the projectile and finish spawning.
		Projectile->DamageEffectSpecHandle = SpecHandle;
		Projectile->FinishSpawning(SpawnTransform);
	}
}
