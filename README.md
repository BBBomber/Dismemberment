# Dynamic Dismemberment System — README

## What It Is

A runtime dismemberment system for Unreal Engine 5, written entirely in C++ with no third-party plugins. Supports both static mesh slicing and skeletal mesh limb detachment, driven by weapon hit detection.

---

## How It Works

A sword weapon sweeps a capsule through space each animation frame during an attack. When it hits something, it packages the impact point, impact normal, and blade direction into a hit struct and passes it to the target if it has the `IDismemberable` interface.

**Static meshes** are sliced by a plane computed from the blade direction and surface normal. On first hit the static mesh is converted to a procedural mesh, then `KismetProceduralMeshLibrary::SliceProceduralMesh` splits it into two physics-simulated pieces. Each piece can be sliced again until a configurable depth limit is reached.

**Skeletal meshes** find the nearest bone to the impact point, walk up the skeleton hierarchy to the nearest defined severance point, hide that bone chain, extract the affected triangles from the render data into a new procedural mesh chunk, spawn that chunk as a separate physics actor, and ragdoll the remaining body. Blood VFX spawns at the severance point and a blood decal is projected onto the ground below.

---

## How to Test

- Left mouse click to swing your sword on the dynamic physics meshes or the Ai character in the scene.
- The Ai Character loses one limb per slice only if the slice was near to a valid severance bone or to a bone that has not already been severed.
  - Right now the spawned chunks do not have mass or angular dampening so they will rotate a lot and behave weird. This can be easily fixed later.
- For the static mesh you can set the slice depth as high as you want and go to town on the mesh. The chamfer cubes in the scene are all dismemberable.

## Current Limitations and Failures

**Skeletal mesh — no cap geometry.**
The severed cross-section is open. The chunk mesh has a hole where it was cut from the body. Generating a correct cap requires proper polygon triangulation of the boundary loop. Fan triangulation from a centroid (the naive approach) fails on any concave cross-section, which is every real limb. This was left out rather than shipped broken.

**Skeletal mesh — chunk visual quality.**
The procedural chunk is built from the render data transformed by the current skinning pose. At skinned extremes the extracted triangles may not perfectly match the visual mesh due to how dominant-bone triangle filtering interacts with shared-influence vertices near joints. Small gaps can appear at the seam.

**Skeletal mesh — no material on the open face.**
Since there is no cap, a gore/flesh material cannot be applied.

**Static mesh — convex collision only.**
Sliced pieces get a single convex hull from all their vertices. Concave pieces get an incorrect hull, causing floating or inaccurate collision.
Requires more testing to improve.

---

## Better Approaches

**Pre-baked modular limb meshes** are what AAA games use. Each limb is a separate skeletal mesh asset sharing the main skeleton. On dismemberment the bone chain hides and the pre-made mesh spawns at the socket. The cap is baked by the artist at authoring time with a proper gore material. Tradeoff is per-character art cost for every limb. And proper weighting is required for each bone.

**Physics constraint breaking** requires no code. Enable `Linear Breakable` / `Angular Breakable` on constraints in the Physics Asset and apply force to the mesh. Works well for non-organic characters where the mesh seam is not visible. Produces no separate geometry or cap.
