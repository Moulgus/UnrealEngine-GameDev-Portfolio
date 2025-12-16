# Gameplay Ability System – Multiplayer Combat Prototype

This project showcases a **multiplayer-ready Gameplay Ability System (GAS)** implementation in Unreal Engine 5.

The focus is on a clean, scalable combat pipeline built for **server-authoritative gameplay**, combining:
- Blueprint-based ability orchestration
- C++ damage calculations
- Attribute-driven stat management
- Projectile-based abilities

---

## Key Features

- **Gameplay Ability: Fireball**
  - Targeting via trace
  - Animation-driven casting
  - Projectile spawned through Gameplay Event
- **Server-authoritative projectile logic**
- **Damage handled via Gameplay Effects + ExecCalc**
- **AttributeSet-based health and stat management**
- **MMC-driven MaxHealth scaling**
- Designed with **multiplayer replication** in mind

---

## Multiplayer Architecture

This project follows GAS best practices for multiplayer:

- Abilities are **activated client-side**, but validated and executed on the **server**
- Projectiles are spawned **only on the server**
- Damage calculations are performed **exclusively on the server**
- Attributes are replicated via GAS
- Gameplay Events synchronize animation and gameplay without timers

This ensures deterministic combat behavior and prevents client-side cheating.

---

## Gameplay Ability – Fireball (Blueprint)

The Fireball ability is implemented as a **Gameplay Ability Blueprint** that coordinates targeting, animation, and projectile spawning.

![Fireball Ability Blueprint](Screenshots/GA_FireBolt.JPG)

### Ability Flow
1. Resolve target location using mouse trace
2. Rotate character toward target
3. Play casting montage
4. Wait for Gameplay Event triggered by AnimNotify
5. Spawn projectile (server-authoritative)
6. End ability cleanly

**Why this matters**
- No hard-coded delays
- Animation and gameplay are fully synchronized
- Clean separation between visuals and logic

**TODO**
- Add short gameplay GIF showing Fireball cast in multiplayer

---

## Gameplay Effect – Damage

Damage is applied using an **Instant Gameplay Effect** that delegates its logic to a custom execution calculation.

![Damage Gameplay Effect](Screenshots/GE_Damage.JPG)

### Key Points
- Gameplay Effect contains no static damage values
- All damage logic lives in C++
- Damage values are passed via **SetByCaller**
- Easily reusable by multiple abilities

---

## Selected C++ Implementation

Below are **key excerpts** from the C++ implementation that form the core of the combat pipeline.

---

### Damage Execution Calculation (ExecCalc_Damage.cpp)

This class is responsible for calculating **final damage on the server**.

**Responsibilities**
- Read captured attributes
- Apply resistances, armor, block, and critical hit logic
- Output final damage to a temporary attribute (`IncomingDamage`)

```cpp
// ExecCalc_Damage.cpp (excerpt)
// Aggregates per-damage-type SetByCaller values, applies resistances, block, armor/penetration,
// critical logic, and outputs final value into IncomingDamage (server-authoritative).

void UExecCalc_Damage::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters Params;
    Params.SourceTags = SourceTags;
    Params.TargetTags = TargetTags;

    // 1) Aggregate damage types (SetByCaller) and apply resistances.
    float Damage = 0.f;

    for (const TTuple<FGameplayTag, FGameplayTag>& Pair : FAuraGameplayTags::Get().DamageTypesToResistances)
    {
        const FGameplayTag DamageTypeTag = Pair.Key;
        const FGameplayTag ResistanceTag = Pair.Value;

        checkf(
            AuraDamageStatics().TagsToCaptureDefs.Contains(ResistanceTag),
            TEXT("Missing resistance capture definition for tag: %s"),
            *ResistanceTag.ToString()
        );

        const FGameplayEffectAttributeCaptureDefinition ResistanceDef =
            AuraDamageStatics().TagsToCaptureDefs[ResistanceTag];

        const float DamageTypeValue = Spec.GetSetByCallerMagnitude(DamageTypeTag);

        float Resistance = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ResistanceDef, Params, Resistance);
        Resistance = FMath::Clamp(Resistance, 0.f, 100.f);

        Damage += DamageTypeValue * (100.f - Resistance) / 100.f;
    }

    // 2) Block: chance-based mitigation + store result in EffectContext.
    float TargetBlockChance = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BlockChanceDef, Params, TargetBlockChance);
    TargetBlockChance = FMath::Max(0.f, TargetBlockChance);

    const bool bBlocked = FMath::RandRange(0, 100) < TargetBlockChance;

    FGameplayEffectContextHandle EffectContext = Spec.GetContext();
    UAuraAbilitySystemLibrary::SetIsBlockedHit(EffectContext, bBlocked);

    if (bBlocked)
    {
        Damage *= 0.5f;
    }

    // 3) Armor / Armor Penetration (scaled by level curves).
    float TargetArmor = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, Params, TargetArmor);
    TargetArmor = FMath::Max(0.f, TargetArmor);

    float SourceArmorPen = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef, Params, SourceArmorPen);
    SourceArmorPen = FMath::Max(0.f, SourceArmorPen);

    const float ArmorPenCoeff = GetArmorPenCoefficientFromCurve(/* Source level */);
    const float EffectiveArmorCoeff = GetEffectiveArmorCoefficientFromCurve(/* Target level */);

    const float EffectiveArmor = TargetArmor * (100.f - SourceArmorPen * ArmorPenCoeff) / 100.f;
    Damage *= (100.f - EffectiveArmor * EffectiveArmorCoeff) / 100.f;

    // 4) Critical hit: chance vs resistance (scaled by level curve) + store result in EffectContext.
    float SourceCritChance = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, Params, SourceCritChance);
    SourceCritChance = FMath::Max(0.f, SourceCritChance);

    float TargetCritResist = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitResistanceDef, Params, TargetCritResist);
    TargetCritResist = FMath::Max(0.f, TargetCritResist);

    const float CritResistCoeff = GetCritResistCoefficientFromCurve(/* Target level */);

    const bool bCriticalHit = FMath::RandRange(0, 100) <
        FMath::Max(0.f, SourceCritChance - TargetCritResist * CritResistCoeff);

    UAuraAbilitySystemLibrary::SetIsCriticalHit(EffectContext, bCriticalHit);

    if (bCriticalHit)
    {
        float SourceCritDamage = 0.f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitDamageDef, Params, SourceCritDamage);
        SourceCritDamage = FMath::Max(0.f, SourceCritDamage);

        Damage = Damage * 2.f + SourceCritDamage;
    }

    // 5) Output: write final result to IncomingDamage (consumed by AttributeSet).
    OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
        UAuraAttributeSet::GetIncomingDamageAttribute(),
        EGameplayModOp::Additive,
        Damage
    ));
}
```
```cpp
// Damage coefficients are data-driven (CurveTable assigned in DA_CharacterClassInfo)
const UCharacterClassInfo* ClassInfo = UAuraAbilitySystemLibrary::GetCharacterClassInfo(SourceAvatarActor);
check(ClassInfo && ClassInfo->DamageCalculationCoefficients);

const FString ContextString = TEXT("DamageCoefficients");
UCurveTable* CoeffTable = ClassInfo->DamageCalculationCoefficients;

// Example: Armor Penetration coefficient (scaled by Source level)
const FRealCurve* ArmorPenCurve = CoeffTable->FindCurve(FName("ArmorPenetration"), ContextString);
check(ArmorPenCurve);

const float SourceLevel = ICombatInterface::Execute_GetPlayerLevel(SourceAvatarActor);
const float ArmorPenCoeff = ArmorPenCurve->Eval(SourceLevel);

// Example: Effective Armor coefficient (scaled by Target level)
const FRealCurve* EffectiveArmorCurve = CoeffTable->FindCurve(FName("EffectiveArmor"), ContextString);
check(EffectiveArmorCurve);

const float TargetLevel = ICombatInterface::Execute_GetPlayerLevel(TargetAvatarActor);
const float EffectiveArmorCoeff = EffectiveArmorCurve->Eval(TargetLevel);


