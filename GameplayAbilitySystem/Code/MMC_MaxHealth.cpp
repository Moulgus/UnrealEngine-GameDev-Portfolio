// Calculates base MaxHealth for a modifier using captured attributes and level scaling.
// Inputs: Vigor (captured attribute) and Player Level (via combat interface).
float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather aggregated gameplay tags from source/target (useful for conditional tuning).
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// Pass tags into evaluation so tag-dependent modifiers can apply.
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// Capture Vigor from the spec and clamp to non-negative to avoid reducing MaxHealth.
	float Vigor = 0.f;
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);
	Vigor = FMath::Max<float>(Vigor, 0.f);

	// Pull player level from the source object through the combat interface for scaling.
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetPlayerLevel();

	// Final MaxHealth formula:
	// Base: 80
	// + Vigor scaling: 2.5 per Vigor point
	// + Level scaling: 10 per player level
	return 80.f + 2.5f * Vigor + 10.f * PlayerLevel;
}
