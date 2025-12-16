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


// TODO: Paste core fragment of ExecCalc_Damage::Execute_Implementation()
// - Read SetByCaller damage
// - Apply armor / resistances / crit / block
// - Add output modifier to OutExecutionOutput
