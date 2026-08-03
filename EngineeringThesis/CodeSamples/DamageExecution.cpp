// Portfolio excerpt from the engineering thesis project.
// Unrelated includes and declarations were omitted for readability.

#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystem/DyplomAttributeSet.h"
#include "DyplomGameplayTags.h"
#include "GameplayEffectExtension.h"

namespace DyplomDamageStatics
{
	struct FDamageCaptureStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(SlashResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(BluntResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(PierceResistance);
		DECLARE_ATTRIBUTE_CAPTUREDEF(ChopResistance);

		FDamageCaptureStatics()
		{
			// Resistances are captured from the target at execution time rather
			// than snapshotted when the effect spec is created.
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, FireResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, LightningResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, SlashResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, BluntResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, PierceResistance, Target, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UDyplomAttributeSet, ChopResistance, Target, false);
		}
	};

	static const FDamageCaptureStatics& Get()
	{
		static FDamageCaptureStatics Statics;
		return Statics;
	}
}

UExecCalc_Damage::UExecCalc_Damage()
{
	const auto& DamageStatics = DyplomDamageStatics::Get();

	RelevantAttributesToCapture.Add(DamageStatics.FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics.LightningResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics.SlashResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics.BluntResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics.PierceResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics.ChopResistanceDef);
}

void UExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FDyplomGameplayTags& Tags = FDyplomGameplayTags::Get();
	const auto& DamageStatics = DyplomDamageStatics::Get();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	auto GetCapturedResistanceByTag = [&](const FGameplayTag& ResistanceTag)
	{
		float ResistanceValue = 0.0f;

		if (ResistanceTag == Tags.Attribute_Resistance_Fire)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.FireResistanceDef, EvalParams, ResistanceValue);
		}
		else if (ResistanceTag == Tags.Attribute_Resistance_Lightning)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.LightningResistanceDef, EvalParams, ResistanceValue);
		}
		else if (ResistanceTag == Tags.Attribute_Resistance_Slash)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.SlashResistanceDef, EvalParams, ResistanceValue);
		}
		else if (ResistanceTag == Tags.Attribute_Resistance_Blunt)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.BluntResistanceDef, EvalParams, ResistanceValue);
		}
		else if (ResistanceTag == Tags.Attribute_Resistance_Pierce)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.PierceResistanceDef, EvalParams, ResistanceValue);
		}
		else if (ResistanceTag == Tags.Attribute_Resistance_Chop)
		{
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
				DamageStatics.ChopResistanceDef, EvalParams, ResistanceValue);
		}

		// Negative resistance represents a weakness. Positive resistance is
		// capped so a valid hit always retains at least five percent damage.
		return FMath::Clamp(ResistanceValue, -1.0f, 0.95f);
	};

	float TotalFinalDamage = 0.0f;

	auto ProcessDamageComponent = [&](const FGameplayTag& DamageTypeTag)
	{
		// Every component is supplied by its Gameplay Tag, allowing one hit
		// to combine physical and elemental damage in the same effect spec.
		const float PreResistDamage =
			Spec.GetSetByCallerMagnitude(DamageTypeTag, false, 0.0f);
		if (PreResistDamage <= 0.0f)
		{
			return;
		}

		const FGameplayTag* ResistanceTag =
			Tags.DamageTypeToResistanceMap.Find(DamageTypeTag);
		if (ResistanceTag == nullptr)
		{
			TotalFinalDamage += PreResistDamage;
			return;
		}

		const float ResistanceValue = GetCapturedResistanceByTag(*ResistanceTag);
		TotalFinalDamage += PreResistDamage * (1.0f - ResistanceValue);
	};

	ProcessDamageComponent(Tags.Damage_Type_Slash);
	ProcessDamageComponent(Tags.Damage_Type_Pierce);
	ProcessDamageComponent(Tags.Damage_Type_Blunt);
	ProcessDamageComponent(Tags.Damage_Type_Chop);
	ProcessDamageComponent(Tags.Damage_Type_Fire);
	ProcessDamageComponent(Tags.Damage_Type_Lightning);

	if (TotalFinalDamage > 0.0f)
	{
		// The Attribute Set consumes the meta attribute and handles health,
		// hit reactions, death, and damage event presentation separately.
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				UDyplomAttributeSet::GetIncomingDamageAttribute(),
				EGameplayModOp::Additive,
				TotalFinalDamage));
	}
}

