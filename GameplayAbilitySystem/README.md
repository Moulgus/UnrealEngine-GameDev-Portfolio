# GAS – Data‑Driven Damage (ExecCalc + CurveTables)

> **Goal:** Present a clean, recruiter‑friendly slice of the project: a **Gameplay Ability** (Fire Bolt) + a **Gameplay Effect** (Damage) driven by a **custom ExecCalc** that reads **CurveTable coefficients** assigned in a **DataAsset**.

---

## Overview

This project uses **Unreal Gameplay Ability System (GAS)** with a **server‑authoritative, multiplayer‑ready** damage pipeline. Damage numbers are computed in C++ via a custom **Execution Calculation** (`ExecCalc_Damage`) and tuned in data using a **CurveTable** referenced by a **CharacterClassInfo DataAsset**.

**What this shows:**

* GAS flow: **Ability → GameplayEvent → Projectile → GameplayEffect → ExecCalc**
* **Data‑driven balancing** via CurveTables (no recompilation needed)
* Clean separation of concerns: data (DA/CurveTable) vs logic (ExecCalc)

---

## Screenshots

### 1) Gameplay Ability – Fire Bolt (Blueprint)

*(Place your annotated Fire Bolt BP screenshot here)*

![GA Fire Bolt](Screenshots/GA_Fire_Bolt.jpg)

### 2) Gameplay Effect – Damage (Blueprint Data‑Only)

*(Place your GE_Damage screenshot here)*

![GE Damage](Screenshots/GE_Damage.jpg)

### 3) CurveTable – Damage Coefficients

*(Place your CurveTable screenshot here – rows like ArmorPenetration/EffectiveArmor/CriticalHitResistance)*

![Damage Coefficient Curves](Screenshots/CT_DamageCalcCoefficients.jpg)

### 4) DataAsset – CharacterClassInfo

*(Optional but recommended: screenshot showing DamageCalculationCoefficients assigned in the DA)*

![CharacterClassInfo DataAsset](Screenshots/DA_CharacterClassInfo.jpg)

---

## Damage Pipeline (high‑level)

1. **GA_FireBolt** acquires a target location (mouse trace / hit result)
2. Ability plays a montage and waits for a **GameplayEvent** tag to spawn the projectile
3. Projectile applies **GE_Damage** on hit
4. **GE_Damage** uses **ExecCalc_Damage** to compute final damage
5. ExecCalc reads **CurveTable coefficients** (by row name) to scale damage based on level/attributes

---

## Data‑Driven Damage Scaling (CurveTable)

Damage coefficients are stored in a `UCurveTable` and evaluated at runtime in `ExecCalc_Damage`.
The table is assigned in **DA_CharacterClassInfo**:

* `DamageCalculationCoefficients` → `CT_DamageCalcCoefficients`

**Curve rows used (examples):**

* `ArmorPenetration` – scales armor penetration by **source level**
* `EffectiveArmor` – scales armor effectiveness by **target level**
* `CriticalHitResistance` – mitigates crit chance by **target level**

This enables designers to tune combat by editing curves without touching C++.

---

## Key Code – ExecCalc_Damage (C++)

> Below are portfolio‑friendly excerpts. Keep them as multiple separate code blocks for readability.

### 1) Retrieve CurveTable from CharacterClassInfo

```cpp
// Retrieve damage coefficient curve table from CharacterClassInfo (DataAsset)
const UCharacterClassInfo* ClassInfo =
    UAuraAbilitySystemLibrary::GetCharacterClassInfo(SourceAvatarActor);

check(ClassInfo && ClassInfo->DamageCalculationCoefficients);

UCurveTable* CoeffTable = ClassInfo->DamageCalculationCoefficients;
const FString ContextString(TEXT("DamageCoefficients"));
```

### 2) Evaluate Armor Penetration (scaled by Source Level)

```cpp
const FRealCurve* ArmorPenCurve =
    CoeffTable->FindCurve(FName("ArmorPenetration"), ContextString);

const float SourceLevel = ICombatInterface::Execute_GetPlayerLevel(SourceAvatarActor);
const float ArmorPenCoeff = ArmorPenCurve ? ArmorPenCurve->Eval(SourceLevel) : 0.f;
```

### 3) Evaluate Effective Armor (scaled by Target Level)

```cpp
const FRealCurve* EffectiveArmorCurve =
    CoeffTable->FindCurve(FName("EffectiveArmor"), ContextString);

const float TargetLevel = ICombatInterface::Execute_GetPlayerLevel(TargetAvatarActor);
const float EffectiveArmorCoeff = EffectiveArmorCurve ? EffectiveArmorCurve->Eval(TargetLevel) : 0.f;
```

### 4) Evaluate Critical Hit Resistance (scaled by Target Level)

```cpp
const FRealCurve* CritResistCurve =
    CoeffTable->FindCurve(FName("CriticalHitResistance"), ContextString);

const float CritResistCoeff = CritResistCurve ? CritResistCurve->Eval(TargetLevel) : 0.f;
```

### 5) Why this approach

* **Balance quickly** by editing curves (no code changes)
* Keep logic deterministic and **server‑authoritative** for multiplayer
* Centralize tuning parameters in a single data source (CurveTable)

