# Engineering Thesis - Action RPG Dungeon Crawler (Work in Progress)

An independently developed Unreal Engine 5 action RPG focused on the
integration of **C++**, **Blueprints**, the **Gameplay Ability System**, a
data-driven inventory, and equipment-dependent combat.

This is my most advanced Unreal Engine project to date. The core player
systems are functional, while enemy development, encounter design, balancing,
and content production are still in progress.

## Project Overview

The project is designed around a preparation-driven dungeon crawler loop.
Weapons, armor, enchantments, consumables, damage types, and resistances are
connected rather than implemented as isolated features.

The player is expected to study the enemies in a location and adjust their
equipment before combat. A weapon determines the available attack abilities
and its damage profile, while armor, passive effects, consumables, and
enchantments can change the player's strengths and resistances.

## Development Scope

- Solo engineering thesis project
- Gameplay architecture and implementation in C++ and Blueprints
- Data-driven configuration through Data Tables, Data Assets, Gameplay Tags,
  Curve Tables, and scalable values
- Gameplay UI implemented with UMG and Widget Controllers
- Network-aware architecture with replicated state and server request paths
- Third-party visual and animation assets used to support the gameplay
  prototype

The project is not presented as a finished multiplayer game. Important systems
were designed with replication in mind, but some multiplayer interactions and
edge cases remain unresolved due to the thesis schedule.

## Current Development Status

### Implemented

- Functional inventory and multi-slot equipment system
- Drag and drop, item moving, swapping, stacking, and stack splitting
- Item tooltips, rarity presentation, equipment UI, and character preview
- Weapon-driven Gameplay Ability granting and removal
- Passive Gameplay Effects from equipped weapons and armor
- Consumable items, including instant restoration, regeneration, and temporary
  attribute effects
- Weighted loot generation for chests
- Weapon and armor enchantment foundations
- Multiple physical and elemental damage types with corresponding resistances
- Player melee combat foundation with combo, charged, movement, and aerial
  abilities
- Replication foundations for attributes, inventory, equipment, and chests

### In Progress

- First melee enemy and its combat behavior
- State Tree behavior, perception, target awareness, and combat tuning
- Encounter design and the practical balance of weaknesses and resistances
- Additional equipment, effects, animation profiles, and combat content

### Planned

- Ranged enemy
- Mage or summoner enemy
- Dragon boss encounter
- Bow combat
- Magic staff abilities
- Broader support for one-handed and two-handed weapon configurations

## Video Showcase Series

The project is documented through short, focused videos rather than a single
long demonstration:

1. [Inventory, Equipment & Random Loot System - Update 1](https://www.youtube.com/watch?v=eB6E8bYDTqU) (1:27)
   Item pickup, basic and expanded tooltips, drag and drop, equipment,
   consumable use, stack splitting, and randomly generated chest loot.
2. [Movement & Melee Combat Abilities - Update 2](https://www.youtube.com/watch?v=CsP9E8AFDZU) (1:02)
   Turn in place, sprint, roll, jump and double jump, primary attack combos,
   air-to-ground attack, and charged power attack.
3. [Combat Damage & Hit Reactions - Update 3](https://www.youtube.com/watch?v=3aogIlNINv4) (0:30)
   Damage confirmation on a combat dummy, visible health reduction, multiple
   attack types, and light, medium, and knockdown hit reactions.

[View the complete Unreal Engine portfolio playlist](https://www.youtube.com/playlist?list=PLxXKv2ZyiFaTjrA4zKqaofoGy1IaJdq8e)

## Gameplay Ability System and Equipment Integration

Equipment is a source of gameplay capabilities rather than a purely visual
layer. Each weapon definition can configure:

- Weapon type and damage type
- Abilities granted while the weapon is equipped
- Input tags associated with those abilities
- Passive Gameplay Effects
- Physical damage scaling
- Attack and animation data
- Equipped weapon actor and attachment sockets
- Base weapon trail and enchantment trail overrides

When a weapon is equipped, the equipment component grants its abilities to the
Ability System Component and stores their handles. Unequipping the weapon
removes those abilities and any active equipment effects through the stored
handles.

This keeps ability availability synchronized with the current loadout and
allows new weapon archetypes to be configured through data instead of being
hard-coded into the player character.

## Combat Abilities

Combat input is routed through Gameplay Tags. The current melee foundation
includes:

- Primary attack combos
- Charged power attacks
- Sprint attacks
- Air-to-ground attacks
- Sprint and roll abilities
- Animation-driven damage windows
- Weapon traces using configurable sockets and trace profiles
- Motion Warping for attack alignment

Weapon attack sets store combo steps, animation montages, damage multipliers,
charged attack parameters, and aerial attack configuration. This separates the
ability logic from the content assigned to a particular weapon type.

## Damage, Resistances, and Enchantments

Damage can contain multiple components, allowing a single hit to combine
physical and elemental damage. The current damage types include:

- Slash
- Pierce
- Blunt
- Chop
- Fire
- Lightning

A custom Gameplay Effect Execution Calculation resolves every damage component
against its matching resistance before applying the final incoming damage.
Armor can modify these resistances, with values scaled by item level and
rarity.

Weapon enchantments can add damage components, apply on-hit Gameplay Effects,
and override Niagara weapon trails. Armor enchantments can apply additional
effects while the item is equipped. Enchantments are stored on item instances,
so individual copies of the same base item can have different properties.

## Inventory and Equipment Architecture

The system separates static item definitions, runtime item instances, stored
inventory entries, equipped entries, gameplay bookkeeping, and UI view data.

- **Data Tables** store presentation and lookup data for item archetypes
- **Definition Assets** contain weapon, armor, consumable, and enchantment
  behavior
- **Item Instances** provide unique IDs, quantity, level, rarity, durability,
  damage bonuses, and enchantments
- **Inventory Component** handles storage, stacking, splitting, movement, and
  item requests
- **Equipment Component** validates equipment slots and integrates equipped
  items with GAS
- **Widget Controllers** translate gameplay state into data consumed by UMG
  widgets

Inventory and equipment are owned by the Player State alongside the Ability
System Component. This provides a consistent gameplay state owner and supports
the project's replication model.

## Loot Chests and Consumables

Loot chests generate their contents from configurable loot table assets. The
generation process supports:

- Weighted item selection
- Weighted rarity selection
- Minimum and maximum world levels
- Configurable item quantities
- Item levels derived from the current world loot level
- Taking individual items or transferring all available loot

Consumables use Gameplay Effects and scalable values. Their definitions can
represent instant restoration, regeneration over time, and temporary bonuses.
Level tiers allow potion strength to follow progression without requiring a
separate implementation for every potion level.

## Networking Foundation

The project uses Unreal Engine networking patterns in the core gameplay state:

- Fast Array replication for inventory, equipment, and chest contents
- Server RPC request paths for inventory and equipment operations
- Server-side validation before authoritative state changes
- Replicated GAS attributes and player progression data
- Replicated enemy death and ragdoll state
- Client-specific loot UI opening and server-authoritative loot transfer

This foundation demonstrates that replication was considered during system
design. It should not be interpreted as a claim that the entire game currently
supports a complete, production-ready multiplayer session.

## Enemy AI - Current Work

The first melee enemy is being developed with AI Perception, a State Tree, and
GAS-based attacks. Its current foundation includes:

- Sight, hearing, and damage perception
- Unaware, investigating, and combat awareness states
- Current target and last-known-location tracking
- Alert propagation between compatible nearby enemies
- State Tree tasks for movement and ability activation
- Configurable melee trace profiles
- Health display, hit reactions, death, and ragdoll presentation

The intended enemy roster consists of a melee enemy, ranged enemy, mage or
summoner, and a dragon boss. Only the melee enemy is currently in active
development; the remaining enemy types are part of the planned scope.

## Technical Highlights

- Unreal Engine 5 C++ and Blueprints
- Gameplay Ability System, Gameplay Effects, Attribute Sets, and Gameplay Tags
- Custom Gameplay Effect Execution Calculation
- Fast Array replication and server RPC workflows
- Enhanced Input routed through Gameplay Tags
- UMG with a Widget Controller presentation layer
- Data Tables, Data Assets, Curve Tables, and scalable values
- State Trees and AI Perception
- Animation Montages, Anim Notifies, Motion Warping, and Niagara
- Diversion version control for the full Unreal Engine project

## Selected C++ Samples

Curated excerpts are available for the project's most relevant engineering
areas:

- [Custom damage execution](CodeSamples/DamageExecution.cpp)
- [Equipment and GAS ability lifecycle](CodeSamples/EquipmentAbilityLifecycle.cpp)
- [Replicated inventory and server-authoritative stack transfer](CodeSamples/README.md)

The samples are trimmed for readability and include additional comments that
explain the architectural decisions. The full Unreal Engine project remains in
Diversion because of its binary asset size.

## What This Project Demonstrates

- Independent design of connected Unreal Engine gameplay systems
- Practical integration of GAS with inventory and equipment
- C++ architecture exposed to Blueprint-authored content
- Separation of static definitions, runtime state, replicated storage, and UI
- Designing combat rules around damage types, resistances, and loadout choices
- Applying replication patterns without overstating the current multiplayer
  completeness
- Planning a larger project in milestones while keeping completed systems
  usable during continued development
