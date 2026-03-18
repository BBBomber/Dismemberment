#include "StaticMeshDismembermentComponent.h"
#include "DismembermentTypes.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "KismetProceduralMeshLibrary.h"

UStaticMeshDismembermentComponent::UStaticMeshDismembermentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStaticMeshDismembermentComponent::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (HitData.SwingID != -1 && HitData.SwingID == LastHitSwingID)
        return;

    LastHitSwingID = HitData.SwingID;

    if (!IDismemberable::Execute_CanBeDismembered(this))
    {
        FadeAndDestroy();
        return;
    }

    if (!bIsProceduralMesh || !ProceduralMesh)
    {
        bIsProceduralMesh = false;
        ProceduralMesh = nullptr;
        ConvertToProceduralMesh();
    }

    if (!ProceduralMesh)
        return;

    FVector PlaneNormal;
    FVector PlanePosition;
    ComputeSlicePlane(HitData, PlaneNormal, PlanePosition);
    DrawDebugSolidPlane(
        GetWorld(),
        FPlane(PlanePosition, PlaneNormal),
        PlanePosition,
        50.f,
        FColor::Red,
        false,
        3.f
    );
    ExecuteSlice(PlanePosition, PlaneNormal, HitData.SwingID);
}

bool UStaticMeshDismembermentComponent::CanBeDismembered_Implementation() const
{
    return SliceDepth > 0;
}

void UStaticMeshDismembermentComponent::ConvertToProceduralMesh()
{
    AActor* Owner = GetOwner();
    if (!Owner)
        return;

    UStaticMeshComponent* StaticMeshComp = Owner->FindComponentByClass<UStaticMeshComponent>();
    if (!StaticMeshComp)
        return;

    if (!ProceduralMesh)
    {
        ProceduralMesh = Owner->FindComponentByClass<UProceduralMeshComponent>();
        if (!ProceduralMesh)
            return;
    }

    ProceduralMesh->SetWorldTransform(StaticMeshComp->GetComponentTransform());

    UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(
        StaticMeshComp,
        0,
        ProceduralMesh,
        false
    );

    CachedConvexVertices.Empty();
    if (UStaticMesh* SourceMesh = StaticMeshComp->GetStaticMesh())
    {
        if (UBodySetup* BodySetup = SourceMesh->GetBodySetup())
        {
            for (const FKConvexElem& ConvexElem : BodySetup->AggGeom.ConvexElems)
                CachedConvexVertices.Add(ConvexElem.VertexData);
        }
    }

    ProceduralMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Owner->SetRootComponent(ProceduralMesh);

    StaticMeshComp->SetHiddenInGame(true);
    StaticMeshComp->SetVisibility(false);
    StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StaticMeshComp->DestroyComponent();

    ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProceduralMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

    bIsProceduralMesh = true;
}

void UStaticMeshDismembermentComponent::ComputeSlicePlane(const FDismembermentHitData& HitData, FVector& OutPlaneNormal, FVector& OutPlanePosition) const
{
    OutPlanePosition = HitData.HitResult.ImpactPoint;
    OutPlaneNormal = HitData.BladeDirection;

    if (OutPlaneNormal.IsNearlyZero())
        OutPlaneNormal = HitData.HitResult.ImpactNormal;
}

void UStaticMeshDismembermentComponent::ExecuteSlice(const FVector& PlanePosition, const FVector& PlaneNormal, int32 SwingID)
{
    if (!ProceduralMesh)
        return;

    UProceduralMeshComponent* OtherHalfMesh = nullptr;
    UKismetProceduralMeshLibrary::SliceProceduralMesh(
        ProceduralMesh, PlanePosition, PlaneNormal, true,
        OtherHalfMesh, EProcMeshSliceCapOption::CreateNewSectionForCap, CapMaterial
    );

    if (!OtherHalfMesh)
        return;

    UMaterialInterface* Mat = ProceduralMesh->GetMaterial(0);

    auto GatherAllVertices = [](UProceduralMeshComponent* MeshComp) -> TArray<FVector>
    {
        TArray<FVector> AllVerts;
        for (int32 i = 0; i < MeshComp->GetNumSections(); i++)
        {
            FProcMeshSection* Section = MeshComp->GetProcMeshSection(i);
            if (Section)
            {
                for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
                    AllVerts.Add(Vertex.Position);
            }
        }
        return AllVerts;
    };

    FActorSpawnParameters SpawnParamsOrig;
    SpawnParamsOrig.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParamsOrig.Name = MakeUniqueObjectName(GetWorld(), AActor::StaticClass(), FName("SlicedPiece_Original"));
    AActor* OrigActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParamsOrig);
    if (!OrigActor)
    {
        OtherHalfMesh->DestroyComponent();
        return;
    }

    UProceduralMeshComponent* OrigMesh = NewObject<UProceduralMeshComponent>(OrigActor, TEXT("OriginalHalfMesh"));
    OrigMesh->bUseComplexAsSimpleCollision = false;
    OrigMesh->SetMobility(EComponentMobility::Movable);
    OrigMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    OrigMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    OrigActor->SetRootComponent(OrigMesh);
    OrigMesh->SetWorldTransform(ProceduralMesh->GetComponentTransform());
    OrigActor->SetActorScale3D(GetOwner()->GetActorScale3D());

    for (int32 i = 0; i < ProceduralMesh->GetNumSections(); i++)
    {
        FProcMeshSection* Section = ProceduralMesh->GetProcMeshSection(i);
        if (Section)
        {
            OrigMesh->SetProcMeshSection(i, *Section);
            UMaterialInterface* SectionMat = ProceduralMesh->GetMaterial(i);
            OrigMesh->SetMaterial(i, SectionMat ? SectionMat : Mat);
        }
    }

    TArray<FVector> OrigVerts = GatherAllVertices(OrigMesh);
    if (OrigVerts.Num() > 0)
    {
        OrigMesh->ClearCollisionConvexMeshes();
        OrigMesh->AddCollisionConvexMesh(OrigVerts);
    }
    OrigMesh->RecreatePhysicsState();
    OrigMesh->RegisterComponent();

    UStaticMeshDismembermentComponent* OrigDismemberComp = NewObject<UStaticMeshDismembermentComponent>(OrigActor);
    OrigDismemberComp->InitFromSlicedMesh(OrigMesh, SliceDepth - 1, CapMaterial, SwingID);
    OrigDismemberComp->RegisterComponent();

    OrigMesh->SetSimulatePhysics(true);
    OrigMesh->WakeRigidBody();

    FActorSpawnParameters SpawnParamsOther;
    SpawnParamsOther.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParamsOther.Name = MakeUniqueObjectName(GetWorld(), AActor::StaticClass(), FName("SlicedPiece_Other"));
    AActor* OtherActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParamsOther);
    if (!OtherActor)
    {
        OrigMesh->DestroyComponent();
        OrigActor->Destroy();
        OtherHalfMesh->DestroyComponent();
        return;
    }

    UProceduralMeshComponent* OtherMesh = NewObject<UProceduralMeshComponent>(OtherActor, TEXT("OtherHalfMesh"));
    OtherMesh->bUseComplexAsSimpleCollision = false;
    OtherMesh->SetMobility(EComponentMobility::Movable);
    OtherMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    OtherMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    OtherActor->SetRootComponent(OtherMesh);
    OtherMesh->SetWorldTransform(OtherHalfMesh->GetComponentTransform());
    OtherActor->SetActorScale3D(GetOwner()->GetActorScale3D());

    for (int32 i = 0; i < OtherHalfMesh->GetNumSections(); i++)
    {
        FProcMeshSection* Section = OtherHalfMesh->GetProcMeshSection(i);
        if (Section)
        {
            OtherMesh->SetProcMeshSection(i, *Section);
            UMaterialInterface* SectionMat = OtherHalfMesh->GetMaterial(i);
            OtherMesh->SetMaterial(i, SectionMat ? SectionMat : Mat);
        }
    }

    TArray<FVector> OtherVerts = GatherAllVertices(OtherMesh);
    if (OtherVerts.Num() > 0)
    {
        OtherMesh->ClearCollisionConvexMeshes();
        OtherMesh->AddCollisionConvexMesh(OtherVerts);
    }
    OtherMesh->RecreatePhysicsState();
    OtherMesh->RegisterComponent();

    UStaticMeshDismembermentComponent* OtherDismemberComp = NewObject<UStaticMeshDismembermentComponent>(OtherActor);
    OtherDismemberComp->InitFromSlicedMesh(OtherMesh, SliceDepth - 1, CapMaterial, SwingID);
    OtherDismemberComp->RegisterComponent();

    OtherMesh->SetSimulatePhysics(true);
    OtherMesh->WakeRigidBody();

    ProceduralMesh->DestroyComponent();
    OtherHalfMesh->DestroyComponent();

    AActor* OriginalOwner = GetOwner();
    if (OriginalOwner)
        OriginalOwner->Destroy();

    SliceDepth--;
}

void UStaticMeshDismembermentComponent::FadeAndDestroy()
{
    AActor* Owner = GetOwner();
    if (Owner)
        Owner->Destroy();
}

void UStaticMeshDismembermentComponent::SetProceduralMesh(UProceduralMeshComponent* InMesh)
{
    ProceduralMesh = InMesh;
    bIsProceduralMesh = false;
}

void UStaticMeshDismembermentComponent::InitFromSlicedMesh(UProceduralMeshComponent* Mesh, int32 Depth, UMaterialInterface* InCapMaterial, int32 SwingID)
{
    ProceduralMesh = Mesh;
    bIsProceduralMesh = true;
    SliceDepth = Depth;
    CapMaterial = InCapMaterial;
    LastHitSwingID = SwingID;
}