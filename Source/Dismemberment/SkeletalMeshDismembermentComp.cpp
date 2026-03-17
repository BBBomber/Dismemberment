#include "SkeletalMeshDismembermentComp.h"
#include "DismembermentTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

USkeletalMeshDismembermentComp::USkeletalMeshDismembermentComp()
{
    PrimaryComponentTick.bCanEverTick = false;

    TArray<FName> DefaultBones = {
        FName("neck_01"),
        FName("upperarm_l"),
        FName("upperarm_r"),
        FName("lowerarm_l"),
        FName("lowerarm_r"),
        FName("hand_l"),
        FName("hand_r"),
        FName("thigh_l"),
        FName("thigh_r"),
        FName("calf_l"),
        FName("calf_r"),
        FName("foot_l"),
        FName("foot_r")
    };

    for (const FName& BoneName : DefaultBones)
    {
        FSeverancePointData Point;
        Point.BoneName = BoneName;
        SeverancePoints.Add(Point);
    }
}

void USkeletalMeshDismembermentComp::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (HitData.SwingID != -1 && HitData.SwingID == LastHitSwingID) return;
    LastHitSwingID = HitData.SwingID;

    if (!CanBeDismembered_Implementation()) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    USkeletalMeshComponent* SkelMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelMesh) return;

    FName HitBone = FindNearestBoneToPoint(SkelMesh, HitData.HitResult.ImpactPoint);
    if (HitBone == NAME_None) return;

    FName SeveranceBone = FindNearestSeverancePoint(SkelMesh, HitBone);
    if (SeveranceBone == NAME_None) return;
    if (DetachedBones.Contains(SeveranceBone)) return;

    HideBoneChain(SkelMesh, SeveranceBone);
    SpawnDetachedChunk(SkelMesh, SeveranceBone);
    RagdollBody(SkelMesh);

    DetachedBones.Add(SeveranceBone);
}

bool USkeletalMeshDismembermentComp::CanBeDismembered_Implementation() const
{
    return DetachedBones.Num() < SeverancePoints.Num();
}

FName USkeletalMeshDismembermentComp::FindNearestSeverancePoint(USkeletalMeshComponent* SkelMesh, FName HitBone) const
{
    FName CurrentBone = HitBone;
    while (CurrentBone != NAME_None)
    {
        for (const FSeverancePointData& Point : SeverancePoints)
        {
            if (Point.BoneName == CurrentBone)
                return CurrentBone;
        }
        CurrentBone = SkelMesh->GetParentBone(CurrentBone);
    }
    return NAME_None;
}

FName USkeletalMeshDismembermentComp::FindNearestBoneToPoint(USkeletalMeshComponent* SkelMesh, const FVector& Point) const
{
    USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
    if (!MeshAsset) return NAME_None;

    const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
    FName NearestBone = NAME_None;
    float NearestDistSq = FLT_MAX;

    for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
    {
        FName BoneName = RefSkeleton.GetBoneName(i);
        float DistSq = FVector::DistSquared(Point, SkelMesh->GetBoneLocation(BoneName));
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            NearestBone = BoneName;
        }
    }

    return NearestBone;
}

void USkeletalMeshDismembermentComp::HideBoneChain(USkeletalMeshComponent* SkelMesh, FName RootBone) const
{
    if (!SkelMesh) return;

    USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
    if (!MeshAsset) return;

    const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
    const int32 RootBoneIndex = RefSkeleton.FindBoneIndex(RootBone);
    if (RootBoneIndex == INDEX_NONE) return;

    SkelMesh->HideBoneByName(RootBone, EPhysBodyOp::PBO_None);

    for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
    {
        if (IsBoneChildOf(RefSkeleton, i, RootBoneIndex))
            SkelMesh->HideBoneByName(RefSkeleton.GetBoneName(i), EPhysBodyOp::PBO_None);
    }
}

bool USkeletalMeshDismembermentComp::IsBoneChildOf(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, int32 AncestorIndex) const
{
    int32 CurrentIndex = RefSkeleton.GetParentIndex(BoneIndex);
    while (CurrentIndex != INDEX_NONE)
    {
        if (CurrentIndex == AncestorIndex) return true;
        CurrentIndex = RefSkeleton.GetParentIndex(CurrentIndex);
    }
    return false;
}

void USkeletalMeshDismembermentComp::SpawnDetachedChunk(USkeletalMeshComponent* SkelMesh, FName SeveranceBone) const
{
    USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
    if (!MeshAsset) return;

    const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
    const int32 SeveranceBoneIndex = RefSkeleton.FindBoneIndex(SeveranceBone);
    if (SeveranceBoneIndex == INDEX_NONE) return;

    TSet<int32> ChunkBoneIndices;
    ChunkBoneIndices.Add(SeveranceBoneIndex);
    for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
    {
        if (IsBoneChildOf(RefSkeleton, i, SeveranceBoneIndex))
            ChunkBoneIndices.Add(i);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* ChunkActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!ChunkActor) return;

    UProceduralMeshComponent* ProcMesh = BuildChunkProceduralMesh(SkelMesh, ChunkBoneIndices, ChunkActor);
    if (!ProcMesh)
    {
        ChunkActor->Destroy();
        return;
    }

    ChunkActor->SetRootComponent(ProcMesh);
    ProcMesh->RegisterComponent();
    ProcMesh->SetSimulatePhysics(true);
    ProcMesh->WakeRigidBody();
}
UProceduralMeshComponent* USkeletalMeshDismembermentComp::BuildChunkProceduralMesh(
    USkeletalMeshComponent* SkelMesh,
    const TSet<int32>& ChunkBoneIndices,
    AActor* ChunkActor) const
{
    USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
    if (!MeshAsset) return nullptr;

    FSkeletalMeshRenderData* RenderData = MeshAsset->GetResourceForRendering();
    if (!RenderData || RenderData->LODRenderData.Num() == 0) return nullptr;

    const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
    const FSkinWeightVertexBuffer* SkinWeightBuffer = LODData.GetSkinWeightVertexBuffer();
    if (!SkinWeightBuffer) return nullptr;

    TArray<uint32> IndexBuffer;
    LODData.MultiSizeIndexContainer.GetIndexBuffer(IndexBuffer);

    const FStaticMeshVertexBuffer& StaticVertBuffer = LODData.StaticVertexBuffers.StaticMeshVertexBuffer;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TMap<uint32, int32> VertexRemap;

    for (int32 SectionIdx = 0; SectionIdx < LODData.RenderSections.Num(); SectionIdx++)
    {
        const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIdx];

        for (uint32 TriIdx = 0; TriIdx < Section.NumTriangles; TriIdx++)
        {
            uint32 BaseIdx = Section.BaseIndex + TriIdx * 3;
            uint32 Idx[3] = {
                IndexBuffer[BaseIdx],
                IndexBuffer[BaseIdx + 1],
                IndexBuffer[BaseIdx + 2]
            };

            bool bAllChunk = true;
            for (int32 i = 0; i < 3; i++)
            {
                int32 MaxWeight = -1;
                int32 DominantLocalBone = 0;
                for (int32 Inf = 0; Inf < (int32)SkinWeightBuffer->GetMaxBoneInfluences(); Inf++)
                {
                    int32 Weight = SkinWeightBuffer->GetBoneWeight(Idx[i], Inf);
                    if (Weight > MaxWeight)
                    {
                        MaxWeight = Weight;
                        DominantLocalBone = SkinWeightBuffer->GetBoneIndex(Idx[i], Inf);
                    }
                }

                int32 GlobalBoneIdx = (DominantLocalBone < Section.BoneMap.Num()) ?
                    Section.BoneMap[DominantLocalBone] : 0;

                if (!ChunkBoneIndices.Contains(GlobalBoneIdx))
                {
                    bAllChunk = false;
                    break;
                }
            }

            if (!bAllChunk) continue;

            int32 LocalIdx[3];
            for (int32 i = 0; i < 3; i++)
            {
                if (int32* Existing = VertexRemap.Find(Idx[i]))
                {
                    LocalIdx[i] = *Existing;
                }
                else
                {
                    FVector PosLocal = FVector(USkeletalMeshComponent::GetSkinnedVertexPosition(
                        SkelMesh, Idx[i], LODData,
                        *const_cast<FSkinWeightVertexBuffer*>(SkinWeightBuffer)
                    ));
                    FVector PosWorld = SkelMesh->GetComponentTransform().TransformPosition(PosLocal);
                    FVector Normal = FVector(StaticVertBuffer.VertexTangentZ(Idx[i]));
                    Normal = SkelMesh->GetComponentTransform().TransformVectorNoScale(Normal);
                    FVector2D UV = FVector2D(StaticVertBuffer.GetVertexUV(Idx[i], 0));

                    int32 NewIdx = Vertices.Num();
                    Vertices.Add(PosWorld);
                    Normals.Add(Normal);
                    UVs.Add(UV);
                    VertexRemap.Add(Idx[i], NewIdx);
                    LocalIdx[i] = NewIdx;
                }
            }

            Triangles.Add(LocalIdx[0]);
            Triangles.Add(LocalIdx[1]);
            Triangles.Add(LocalIdx[2]);
        }
    }

    if (Vertices.Num() == 0) return nullptr;

    UProceduralMeshComponent* ProcMesh = NewObject<UProceduralMeshComponent>(ChunkActor, TEXT("ChunkProcMesh"));
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->SetMobility(EComponentMobility::Movable);
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProcMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    ProcMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
    ProcMesh->SetMaterial(0, SkelMesh->GetMaterial(0));

    ProcMesh->ClearCollisionConvexMeshes();
    ProcMesh->AddCollisionConvexMesh(Vertices);
    ProcMesh->RecreatePhysicsState();

    ChunkActor->SetActorLocation(Vertices[0]);

    return ProcMesh;
}


void USkeletalMeshDismembermentComp::RagdollBody(USkeletalMeshComponent* SkelMesh) const
{
    if (!SkelMesh) return;

    SkelMesh->SetAllBodiesSimulatePhysics(true);
    SkelMesh->SetSimulatePhysics(true);

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (Character)
    {
        Character->GetCharacterMovement()->DisableMovement();
        Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}