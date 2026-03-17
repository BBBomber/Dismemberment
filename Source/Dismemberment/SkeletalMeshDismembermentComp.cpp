#include "SkeletalMeshDismembermentComp.h"
#include "DismembermentTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkeletalMergingLibrary.h"      
#include "SkeletalMeshMerge.h"

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

    TArray<FName> ChunkBoneNames;
    ChunkBoneNames.Add(SeveranceBone);
    for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
    {
        if (IsBoneChildOf(RefSkeleton, i, SeveranceBoneIndex))
            ChunkBoneNames.Add(RefSkeleton.GetBoneName(i));
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* ChunkActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!ChunkActor) return;

    USkeletalMeshComponent* ChunkSkelComp = NewObject<USkeletalMeshComponent>(ChunkActor, TEXT("ChunkMesh"));
    ChunkSkelComp->SetSkeletalMesh(MeshAsset);
    ChunkSkelComp->SetPhysicsAsset(SkelMesh->GetPhysicsAsset());
    ChunkActor->SetRootComponent(ChunkSkelComp);
    ChunkSkelComp->SetCollisionProfileName(TEXT("Ragdoll"));
    ChunkSkelComp->RegisterComponent();
    ChunkSkelComp->SetWorldTransform(SkelMesh->GetComponentTransform());

    for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
    {
        FName BoneName = RefSkeleton.GetBoneName(i);
        if (!ChunkBoneNames.Contains(BoneName))
            ChunkSkelComp->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
    }
    ChunkSkelComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ChunkSkelComp->RecreatePhysicsState();
    ChunkSkelComp->SetAllBodiesSimulatePhysics(true);
    ChunkSkelComp->SetSimulatePhysics(true);
    ChunkSkelComp->WakeAllRigidBodies();
}
USkeletalMesh* USkeletalMeshDismembermentComp::BuildChunkSkeletalMesh(
    USkeletalMeshComponent* SkelMesh,
    FName SeveranceBone,
    const TArray<FName>& ChunkBoneNames) const
{
    USkeletalMesh* SourceMesh = SkelMesh->GetSkeletalMeshAsset();
    if (!SourceMesh) return nullptr;

    USkeletalMesh* MergedMesh = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, RF_Transient);
    if (!MergedMesh) return nullptr;

    TArray<USkeletalMesh*> MeshList;
    MeshList.Add(SourceMesh);

    TArray<FSkelMeshMergeSectionMapping> SectionMappings;

    FSkeletalMeshMerge Merger(MergedMesh, MeshList, SectionMappings, 0);
    if (!Merger.DoMerge())
    {
        UE_LOG(LogTemp, Error, TEXT("FSkeletalMeshMerge failed for chunk %s"), *SeveranceBone.ToString());
        return nullptr;
    }

    return MergedMesh;
}

void USkeletalMeshDismembermentComp::RagdollBody(USkeletalMeshComponent* SkelMesh) const
{
    if (!SkelMesh) return;

    SkelMesh->SetAllBodiesSimulatePhysics(true);
    SkelMesh->SetSimulatePhysics(true);

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (Character)
        Character->GetCharacterMovement()->DisableMovement();
}