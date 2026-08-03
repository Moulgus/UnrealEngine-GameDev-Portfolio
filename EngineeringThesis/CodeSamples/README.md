# Selected C++ Samples

These excerpts come from the engineering thesis project and focus on three
connected parts of its gameplay architecture: damage calculation, the
equipment-to-GAS lifecycle, and replicated inventory state.

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

