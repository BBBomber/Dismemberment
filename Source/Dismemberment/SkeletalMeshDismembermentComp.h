#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "SkeletalMeshDismembermentComp.generated.h"

class UProceduralMeshComponent;

USTRUCT(BlueprintType)
struct FSeverancePointData
{
    GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
        FName BoneName;
};

UCLASS(ClassGroup = (Dismemberment), meta = (BlueprintSpawnableComponent))
class DISMEMBERMENT_API USkeletalMeshDismembermentComp : public UActorComponent, public IDismemberable
{
    GENERATED_BODY()

public:

    USkeletalMeshDismembermentComp();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
        TArray<FSeverancePointData> SeverancePoints;

    virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;
    virtual bool CanBeDismembered_Implementation() const override;

private:

    TSet<FName> DetachedBones;
    int32 LastHitSwingID = -1;

    FName FindNearestSeverancePoint(USkeletalMeshComponent* SkelMesh, FName HitBone) const;
    FName FindNearestBoneToPoint(USkeletalMeshComponent* SkelMesh, const FVector& Point) const;
    void HideBoneChain(USkeletalMeshComponent* SkelMesh, FName RootBone) const;
    bool IsBoneChildOf(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, int32 AncestorIndex) const;
    void SpawnDetachedChunk(USkeletalMeshComponent* SkelMesh, FName SeveranceBone) const;
    UProceduralMeshComponent* BuildChunkProceduralMesh(USkeletalMeshComponent* SkelMesh, const TSet<int32>& ChunkBoneIndices, AActor* ChunkActor) const;
    void RagdollBody(USkeletalMeshComponent* SkelMesh) const;
};