# Building Drone: behavior and implementation

Describes the retained payload **0.2.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.2.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.2.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

A $2,000 controller is sold through Shop Framework after the native Gravity Boots progression requirement. Assign it through normal Equipment inventory. Alt+D deploys or returns a detached building camera; select the normal hammer while deployed to use native construction. The drone claims shared camera resources, uses configured speed/range, and keeps the real player at its captured location rather than treating the camera as a second player.

## State and lifecycle

Ownership is keyed by world identity in the INI owned-worlds section. Purchase verifies progression and native payment; a failed ownership write leaves the purchase valid for that session and reports failure. Deployment is temporary. Return, input loss and world changes restore rooted camera/player state; incomplete restoration retains ownership of the camera service for retry.

## Integration and configuration

Runtime dependencies: [shop-framework](https://github.com/swooping-mods/shop-framework), [equipment-framework](https://github.com/swooping-mods/equipment-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/building-drone/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `shop-framework.shop` | 1 | at least 56 bytes |
| Requires `equipment-framework.equipment` | 1 | at least 32 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

Packaged defaults in [settings.ini](../source/mods/building-drone/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `[drone]` | `price` | `2000` |
| `[drone]` | `speed` | `12` |
| `[drone]` | `range` | `80` |

## Implementation sources

- [mod.c](../source/mods/building-drone/mod.c)
- [common.h](../source/mods/building-drone/common.h)
- [camera_restore.h](../source/mods/building-drone/camera_restore.h)
- [storage.h](../source/mods/building-drone/storage.h)

## Verification boundary

No autonomous construction, saved drone entity or independent multiplayer avatar. Native purchase checks do not qualify detached flight, building reach or ownership reload.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
