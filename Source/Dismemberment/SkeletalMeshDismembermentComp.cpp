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
#include "Kismet/GameplayStatics.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

USkeletalMeshDismembermentComp::USkeletalMeshDismembermentComp()
{
    PrimaryComponentTick.bCanEverTick = false;

    TArray<FSeverancePointData> Defaults = {
       { FName("pelvis"),      FName("spine_01") },
       { FName("spine_01"),    NAME_None },
       { FName("spine_02"),    NAME_None },
       { FName("spine_03"),    NAME_None },
       { FName("neck_01"),     NAME_None },
       { FName("upperarm_l"),  NAME_None },
       { FName("upperarm_r"),  NAME_None },
       { FName("lowerarm_l"),  NAME_None },
       { FName("lowerarm_r"),  NAME_None },
       { FName("hand_l"),      NAME_None },
       { FName("hand_r"),      NAME_None },
       { FName("thigh_l"),     NAME_None },
       { FName("thigh_r"),     NAME_None },
       { FName("calf_l"),      NAME_None },
       { FName("calf_r"),      NAME_None },
       { FName("foot_l"),      NAME_None },
       { FName("foot_r"),      NAME_None },
    };

  SeverancePoints = Defaults;
}
void USkeletalMeshDismembermentComp::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (HitData.SwingID != -1 && HitData.SwingID == LastHitSwingID)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: duplicate SwingID %d, skipping"), HitData.SwingID);
        return;
    }
    LastHitSwingID = HitData.SwingID;

    if (!CanBeDismembered_Implementation())
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: CanBeDismembered returned false"));
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: no owner"));
        return;
    }

    USkeletalMeshComponent* SkelMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: no SkeletalMeshComponent on %s"), *Owner->GetName());
        return;
    }

    FName HitBone = FindNearestBoneToPoint(SkelMesh, HitData.HitResult.ImpactPoint);
    UE_LOG(LogTemp, Warning, TEXT("ProcessHit: ImpactPoint %s, nearest bone: %s"), *HitData.HitResult.ImpactPoint.ToString(), *HitBone.ToString());
    if (HitBone == NAME_None) return;

    FName SeveranceBone = FindNearestSeverancePoint(SkelMesh, HitBone);
    UE_LOG(LogTemp, Warning, TEXT("ProcessHit: severance bone: %s"), *SeveranceBone.ToString());
    if (SeveranceBone == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: no severance point found in chain from %s"), *HitBone.ToString());
        return;
    }

    if (DetachedBones.Contains(SeveranceBone))
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit: %s already detached"), *SeveranceBone.ToString());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ProcessHit: dismembering %s"), *SeveranceBone.ToString());
    HideBoneChain(SkelMesh, SeveranceBone);
    SpawnDetachedChunk(SkelMesh, SeveranceBone);
    SpawnCapDecal(SkelMesh, SeveranceBone);
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
            {
                if (Point.RemapToSeveranceBone != NAME_None)
                    return Point.RemapToSeveranceBone;
                return CurrentBone;
            }
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

    FVector Center;
    UProceduralMeshComponent* ProcMesh = BuildChunkProceduralMesh(SkelMesh, ChunkBoneIndices, ChunkActor, Center);
    if (!ProcMesh)
    {
        ChunkActor->Destroy();
        return;
    }

    ChunkActor->SetRootComponent(ProcMesh);
    ProcMesh->RegisterComponent();
    ChunkActor->SetActorLocation(Center);
    ProcMesh->SetSimulatePhysics(true);
    ProcMesh->WakeRigidBody();
}
UProceduralMeshComponent* USkeletalMeshDismembermentComp::BuildChunkProceduralMesh(
    USkeletalMeshComponent* SkelMesh,
    const TSet<int32>& ChunkBoneIndices,
    AActor* ChunkActor,
    FVector& OutCenter) const
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

    FBox Bounds(Vertices);
    FVector Center = Bounds.GetCenter();
    OutCenter = Center;

    TArray<FVector> LocalVertices;
    for (const FVector& V : Vertices)
        LocalVertices.Add(V - Center);

    UProceduralMeshComponent* ProcMesh = NewObject<UProceduralMeshComponent>(ChunkActor, TEXT("ChunkProcMesh"));
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->SetMobility(EComponentMobility::Movable);
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProcMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    ProcMesh->CreateMeshSection_LinearColor(0, LocalVertices, Triangles, Normals, UVs, Colors, Tangents, true);
    ProcMesh->SetMaterial(0, SkelMesh->GetMaterial(0));

    // find boundary edges
    TMap<TPair<int32, int32>, int32> EdgeCount;
    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        int32 A = Triangles[i], B = Triangles[i + 1], C = Triangles[i + 2];
        auto AddEdge = [&](int32 V0, int32 V1)
        {
            TPair<int32, int32> Edge(FMath::Min(V0, V1), FMath::Max(V0, V1));
            EdgeCount.FindOrAdd(Edge)++;
        };
        AddEdge(A, B);
        AddEdge(B, C);
        AddEdge(C, A);
    }

    TArray<TPair<int32, int32>> BoundaryEdges;
    for (auto& Pair : EdgeCount)
    {
        if (Pair.Value == 1)
            BoundaryEdges.Add(Pair.Key);
    }
    UE_LOG(LogTemp, Warning, TEXT("Boundary edges found: %d"), BoundaryEdges.Num());
    

    if (BoundaryEdges.Num() > 0)
    {
        TSet<TPair<int32, int32>> Remaining(BoundaryEdges);
        TPair<int32, int32> Current = BoundaryEdges[0];
        Remaining.Remove(Current);
        TArray<int32> Loop;
        Loop.Add(Current.Key);
        Loop.Add(Current.Value);
        int32 LastVert = Current.Value;

        while (Remaining.Num() > 0)
        {
            bool bFound = false;
            for (auto& Edge : Remaining)
            {
                if (Edge.Key == LastVert)
                {
                    Loop.Add(Edge.Value);
                    LastVert = Edge.Value;
                    Remaining.Remove(Edge);
                    bFound = true;
                    break;
                }
                else if (Edge.Value == LastVert)
                {
                    Loop.Add(Edge.Key);
                    LastVert = Edge.Key;
                    Remaining.Remove(Edge);
                    bFound = true;
                    break;
                }
            }
            if (!bFound) break;
        }
        UE_LOG(LogTemp, Warning, TEXT("Loop size: %d"), Loop.Num());
        UE_LOG(LogTemp, Warning, TEXT("Boundary edges found: %d"), BoundaryEdges.Num());
        FVector Centroid = FVector::ZeroVector;
        for (int32 Idx : Loop)
            Centroid += LocalVertices[Idx];
        Centroid /= Loop.Num();

        TArray<FVector> CapVerts;
        TArray<int32> CapTris;
        TArray<FVector> CapNormals;
        TArray<FVector2D> CapUVs;

        CapVerts.Add(Centroid);
        CapNormals.Add(FVector::UpVector);
        CapUVs.Add(FVector2D(0.5f, 0.5f));

        for (int32 i = 0; i < Loop.Num(); i++)
        {
            CapVerts.Add(LocalVertices[Loop[i]]);
            CapNormals.Add(FVector::UpVector);
            CapUVs.Add(FVector2D(0.f, 0.f));

            int32 Next = (i + 1) % Loop.Num();
            CapTris.Add(0);
            CapTris.Add(Next + 1);
            CapTris.Add(i + 1);
        }

        TArray<FLinearColor> CapColors;
        TArray<FProcMeshTangent> CapTangents;
        ProcMesh->CreateMeshSection_LinearColor(1, CapVerts, CapTris, CapNormals, CapUVs, CapColors, CapTangents, false);
        ProcMesh->SetMaterial(1, SkelMesh->GetMaterial(0));
    }

    ProcMesh->ClearCollisionConvexMeshes();
    ProcMesh->AddCollisionConvexMesh(LocalVertices);
    ProcMesh->RecreatePhysicsState();

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

void USkeletalMeshDismembermentComp::OnChunkDestroyed(AActor* DestroyedActor)
{
    UE_LOG(LogTemp, Error, TEXT("Chunk destroyed: %s"), *DestroyedActor->GetName());
}

void USkeletalMeshDismembermentComp::SpawnCapDecal(USkeletalMeshComponent* SkelMesh, FName SeveranceBone) const
{
    if (!CapDecalMaterial) return;

    const FVector BoneLocation = SkelMesh->GetBoneLocation(SeveranceBone);
    const FRotator BoneRotation = SkelMesh->GetBoneQuaternion(SeveranceBone).Rotator();
    const FVector DecalSize = FVector(10.f, 15.f, 15.f);

    UGameplayStatics::SpawnDecalAtLocation(
        GetWorld(),
        CapDecalMaterial,
        DecalSize,
        BoneLocation,
        BoneRotation,
        0.0f
    );
}