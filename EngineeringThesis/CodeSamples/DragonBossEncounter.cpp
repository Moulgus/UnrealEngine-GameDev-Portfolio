// Portfolio excerpt from ADyplomDragonBossCharacter.
// State Tree tasks execute individual attacks; the character owns the
// authoritative phase, aerial-objective, and reset state shown here.

void ADyplomDragonBossCharacter::EvaluatePhaseThresholds()
{
	if (!HasAuthority() || !bBossEncounterActive || IsDead() ||
		bPhaseTransitionPending)
	{
		return;
	}

	const float MaxHealth = GetMaxHealth();
	const float HealthFraction = MaxHealth > 0.0f
		? FMath::Clamp(GetHealth() / MaxHealth, 0.0f, 1.0f)
		: 0.0f;

	if (BossPhase == EDyplomDragonBossPhase::PhaseOne &&
		HealthFraction <= PhaseTwoHealthThreshold)
	{
		RequestPhaseTransition(EDyplomDragonBossPhase::PhaseTwo);
		return;
	}

	if (BossPhase == EDyplomDragonBossPhase::PhaseTwo &&
		HealthFraction <= PhaseThreeHealthThreshold)
	{
		// A minimum phase duration prevents two health thresholds from
		// collapsing into one transition after a burst of damage.
		const UWorld* World = GetWorld();
		const float Now = World != nullptr ? World->GetTimeSeconds() : 0.0f;
		if (Now >= EarliestNextPhaseTransitionTime)
		{
			RequestPhaseTransition(EDyplomDragonBossPhase::PhaseThree);
		}
		else
		{
			GetWorldTimerManager().SetTimer(
				PhaseEvaluationTimerHandle,
				this,
				&ThisClass::EvaluatePhaseThresholds,
				EarliestNextPhaseTransitionTime - Now,
				false);
		}
	}
}

bool ADyplomDragonBossCharacter::AreAerialObjectivesComplete() const
{
	if (!bAerialIntermissionActive)
	{
		return false;
	}

	const FDyplomDragonAerialPhaseRequirements& Required =
		GetActiveAerialRequirements();

	// Landing is gated by both time and encounter content. The dragon cannot
	// leave living summons, an active meteor shower, or an unfinished attack
	// quota behind simply because a timer expired.
	return GetAerialIntermissionElapsedTime() >=
			FMath::Max(0.0f, Required.MinimumAirDuration) &&
		CompletedAerialSummonWaves >=
			FMath::Max(0, Required.MinimumCompletedSummonWaves) &&
		GetLivingAerialMinionCount() == 0 &&
		!bAerialSummonWaveActive &&
		!bMeteorRequiredAfterSummonWave &&
		!bTrackedMeteorShowerActive &&
		AerialFireballCount >= FMath::Max(0, Required.MinimumFireballCount) &&
		AerialBarrageCount >= FMath::Max(0, Required.MinimumBarrageCount) &&
		AerialSpreadFireCount >= FMath::Max(0, Required.MinimumSpreadFireCount) &&
		AerialFlyoverCount >= FMath::Max(0, Required.MinimumFlyoverCount) &&
		AerialMeteorRainCount >= FMath::Max(0, Required.MinimumMeteorRainCount);
}

bool ADyplomDragonBossCharacter::TryRequestPhaseThreeAirSequence(
	AActor* TargetActor)
{
	if (!HasAuthority() || BossPhase != EDyplomDragonBossPhase::PhaseThree ||
		!IsValid(TargetActor) || IsDragonAirborne() ||
		bPhaseThreeAirSequenceRequested || bPhaseThreeAirSequenceActive)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (Now < NextPhaseThreeAirSequenceDecisionTime)
	{
		return false;
	}

	// A failed probability roll is throttled too. Otherwise a per-tick check
	// would make an apparently optional sequence practically guaranteed.
	NextPhaseThreeAirSequenceDecisionTime =
		Now + FMath::Max(0.1f, PhaseThreeAirSequenceDecisionRetryDelay);
	if (FMath::FRand() > FMath::Clamp(PhaseThreeAirSequenceStartChance, 0.0f, 1.0f))
	{
		return false;
	}

	bPhaseThreeAirSequenceRequested = true;
	return true;
}

bool ADyplomDragonBossCharacter::ChoosePhaseThreeAirContinuation()
{
	if (!HasAuthority() || BossPhase != EDyplomDragonBossPhase::PhaseThree ||
		!bPhaseThreeAirSequenceActive ||
		DragonMovementState != EDyplomDragonMovementState::Flying)
	{
		bContinuePhaseThreeAirSequence = false;
		return false;
	}

	const int32 ActionLimit = FMath::Max(1, PhaseThreeMaximumConsecutiveAirActions);
	bContinuePhaseThreeAirSequence = PhaseThreeAirActionCount < ActionLimit &&
		FMath::FRand() <= FMath::Clamp(PhaseThreeContinueAirChance, 0.0f, 1.0f);
	return bContinuePhaseThreeAirSequence;
}

bool ADyplomDragonBossCharacter::BeginEncounterReset()
{
	if (!HasAuthority() || IsDead())
	{
		return false;
	}

	DestroyTrackedEncounterActors(); // Removes summons and hazards without loot.
	SetBossEncounterActive(false);
	CancelTacticalRelocation(false);
	CancelPhaseThreeAirSequence(false);
	ResetAerialProgress();
	bEncounterResetInProgress = true;
	return true;
}
