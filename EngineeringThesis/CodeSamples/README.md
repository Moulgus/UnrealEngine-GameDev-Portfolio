# Selected C++ Samples

These excerpts come from the engineering thesis project and focus on connected
parts of its gameplay architecture: damage calculation, the equipment-to-GAS
lifecycle, replicated inventory, quick-wheel item references, enchantment
procs, assisted-random loot, the dragon encounter, and persistent runs.

The full Unreal Engine project is maintained in Diversion because it contains
large binary assets. The samples are lightly trimmed to remove unrelated
boilerplate and include additional English comments for portfolio readability.
The gameplay logic remains consistent with the project implementation.

## Damage Execution

[View `DamageExecution.cpp`](DamageExecution.cpp)

A custom Gameplay Effect Execution Calculation reads damage components from
SetByCaller magnitudes, maps every damage type to a resistance attribute, and
accumulates the result into the `IncomingDamage` meta attribute.

## Equipment and GAS Lifecycle

[View `EquipmentAbilityLifecycle.cpp`](EquipmentAbilityLifecycle.cpp)

Equipping a weapon grants its configured abilities and passive Gameplay
Effects. Their handles are stored per item instance so the exact gameplay state
can be removed safely when that weapon is unequipped or visual setup fails.

## Replicated Inventory

- [View `ReplicatedInventoryFastArray.h`](ReplicatedInventoryFastArray.h)
- [View `ServerAuthoritativeStackTransfer.cpp`](ServerAuthoritativeStackTransfer.cpp)

Inventory entries use Unreal Engine's Fast Array Serializer for delta
replication. Mutating operations are validated on the authority, mark only the
affected entries as dirty, and return the confirmed transfer amount to the
requesting client.

## Favorites Quick Wheel

[View `QuickWheelFavorites.cpp`](QuickWheelFavorites.cpp)

The quick wheel distinguishes stackable item variants from unique equipment,
resolves favorites across inventory and equipped storage, and routes activation
through server-authoritative item operations.

## Data-Driven Enchantment Procs

[View `WeaponEnchantmentProcs.cpp`](WeaponEnchantmentProcs.cpp)

One configurable proc pipeline supports periodic damage, threshold buildup,
chain damage, Gameplay Tag gates, scalable values, GAS effects, and UI buildup
tracking without requiring a separate ability class for every enchantment.

## Assisted-Random Loot Bag

[View `LootBagGeneration.cpp`](LootBagGeneration.cpp)

Chests sharing a `LootTableID` draw categories from one finite runtime bag.
Successful rolls consume tokens, exhausted bags refill, and the authoritative
generator then resolves rarity, weighted item selection, level, quantity, and
enchantments into a unique item instance.

## Dragon Encounter State

[View `DragonBossEncounter.cpp`](DragonBossEncounter.cpp)

The boss character owns health-based phases, objective-gated aerial
intermissions, capped probability-driven Phase 3 air sequences, and encounter
cleanup. Individual attacks remain separate State Tree and GAS actions.

## Save and New-Run Transition

[View `SaveAndNewRun.cpp`](SaveAndNewRun.cpp)

The save subsystem distinguishes restoring the current world from beginning a
fresh run. A new run carries player progression, item instances, equipment, and
favorites into a clean map, while ordinary loads reconstruct stable-ID world
state such as completed arenas, chest contents, and doors.
