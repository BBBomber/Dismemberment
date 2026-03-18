# Dynamic Dismemberment System — README

## What It Is

A runtime dismemberment system for Unreal Engine 5, written entirely in C++ with no third-party plugins. Supports both static mesh slicing and skeletal mesh limb detachment, driven by weapon hit detection. Currently does not support cap creation on skeleton mesh limbs.


## How It Works

During the players swing animation, the weaapon(has base and tip socket) runs a sweep trace called from an anim state(start is prev base and end is current tip socket).

When it hits an object implementing the IDismemberable interface, it gets the cut plane using the cross product of the base to tip dir and the tip delta dir. The weapon passes this onto the static/skeleton dismemberment comp.

If it hit a static mesh, the mesh gets split along that plane into two pieces that fall apart and can be cut again until slice depth on the new actors hit 0. (configurable in bp)

If it hit a skeletal mesh, it finds the nearest bone, walks up to the nearest valid cut point, hides that part of the body, builds a matching chunk from the mesh data and spawns it as a separate flying piece, then drops the body into ragdoll. Blood spawns at the cut point. The ragdoll can continued to be cut up until it has no severable bones left. Currently this does not support a cap mesh on the spawned limb chunk. 


## How to Test

- Left mouse click to swing your sword on the dynamic physics meshes(ones with the rock material) or the Ai character in the scene.

- The Ai Character loses one limb per slice only if the slice was near to a valid severance bone or to a bone that has not already been severed.
  - Right now the spawned chunks do not have mass or angular dampening so they will rotate a lot and behave weird. This can be easily fixed by adding mass and angular dampening or substepping.
    
- For the static mesh you can set the slice depth as high as you want and go to town on the mesh.
- Push the mesh after cutting it with your body since no extra impulse is currently imparted by the cut so the pieces will remain to make the whole shape until pushed.
- there are debugs to show the cut plane and sword sweep.

## Current Limitations and Failures

**Skeletal mesh, no cap geometry.**
The severed cross-section is open. The chunk mesh has a hole where it was cut from the body. Generating a correct cap requires proper polygon triangulation of the boundary loop. Or a way to make the boundary loop flat.

**Static mesh, convex collision only.**
Sliced pieces get a single convex hull from all their vertices. Concave pieces get an incorrect hull, causing floating or inaccurate collision.
Requires more testing to improve.


## Alternative Approaches

**Pre-baked modular limb meshes** are what AAA games use. Each limb is a separate skeletal mesh asset sharing the main skeleton. On dismemberment the bone chain hides and the pre-made mesh spawns at the socket. The cap is baked by the artist at authoring time with a proper gore material. Tradeoff is per-character art cost for every limb. And proper weighting is required for each bone.

**Physics constraint breaking** requires no code. Enable `Linear Breakable` / `Angular Breakable` on constraints in the Physics Asset and apply force to the mesh. Works well for non-organic characters where the mesh seam is not visible. Produces no separate geometry or cap.
