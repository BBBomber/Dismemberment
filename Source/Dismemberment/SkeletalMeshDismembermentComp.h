#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"
#include "SkeletalMeshDismembermentComp.generated.h"

class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FSeverancePointData
{
    GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
        FName BoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
        FName RemapToSeveranceBone = NAME_None;
};

UCLASS(ClassGroup = (Dismemberment), meta = (BlueprintSpawnableComponent))
class DISMEMBERMENT_API USkeletalMeshDismembermentComp : public UActorComponent, public IDismemberable
{
    GENERATED_BODY()

public:

    //set up default severance points for the defaul quin/manny mannequin
    //each entry acts as a cut point
    //remap redirects hits to valid bones pretty much
    USkeletalMeshDismembermentComp();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
        TArray<FSeverancePointData> SeverancePoints;

    //not set yet
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment|VFX")
        UNiagaraSystem* BloodEffect = nullptr;
    //not set yet 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment|VFX")
        UMaterialInterface* BloodDecalMaterial = nullptr;


    //finds nerest bone to contact
    //walks to nearest severance point in heriarchy, 
        //if not detatched, hide bone chain
        //spawn chunk
        //spawn effects(not implemented)
        //ragdoll main body :). keep cutting it up :)
    virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;

    //true while detached bones less than severance points
    virtual bool CanBeDismembered_Implementation() const override;

private:

    TSet<FName> DetachedBones;
    int32 LastHitSwingID = -1;

    //starts at hit bones
    //wals up chain to parent bone and checks against severance points, returns remap if set otherwise self
    //name none is no match return
    FName FindNearestSeverancePoint(USkeletalMeshComponent* SkelMesh, FName HitBone) const;

    //iterates bones in ref skelly, computes sq dist from impact to bone world loc, return closest
    FName FindNearestBoneToPoint(USkeletalMeshComponent* SkelMesh, const FVector& Point) const;

    //hide this and decendents 
    void HideBoneChain(USkeletalMeshComponent* SkelMesh, FName RootBone) const;

    //wals up index and true if ancestor found
    bool IsBoneChildOf(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, int32 AncestorIndex) const;
    
    //severance bone + all children
    //spawn actor and calls build prcedural mesh for the chunk and result is the root of new actor
    //positions actor to chunk center and enables physics sim for it
    void SpawnDetachedChunk(USkeletalMeshComponent* SkelMesh, FName SeveranceBone) const;
    
    // reads the raw triangle data the GPU uses to draw the mesh
    // for each triangle, finds which bone has the most influence over each of its 3 corners
    // if all 3 corners are dominated by bones in the limb being cut off, that triangle is part of the chunk
    // GetSkinnedVertexPosition applies the current animation pose so the chunk appears in the right position, not T-pose
    // normals are read separately and rotated to match the mesh's position in the world
    // all vertices are shifted so the center of the chunk sits at its actor's origin, otherwise physics behaves wrong
    // builds a single procedural mesh section and wraps a convex collision hull (rubber band/inflated balloon around triangles for smple col) around all vertices
    UProceduralMeshComponent* BuildChunkProceduralMesh(USkeletalMeshComponent* SkelMesh, const TSet<int32>& ChunkBoneIndices, AActor* ChunkActor, FVector& OutCenter) const;
    void RagdollBody(USkeletalMeshComponent* SkelMesh) const;
    
    //vfx + decal below cut - for when I get to adding these
    void SpawnBloodEffects(USkeletalMeshComponent* SkelMesh, FName SeveranceBone) const;
};