#include "DismemberableActor.h"

ADismemberableActor::ADismemberableActor()
{
    PrimaryActorTick.bCanEverTick = false;
    DismembermentComp = CreateDefaultSubobject<UStaticMeshDismembermentComponent>(TEXT("DismembermentComp"));
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    ProceduralMesh->SetupAttachment(RootComponent);
}

void ADismemberableActor::BeginPlay()
{
    Super::BeginPlay();
    DismembermentComp->SetProceduralMesh(ProceduralMesh);
}

void ADismemberableActor::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (DismembermentComp)
        IDismemberable::Execute_ProcessHit(DismembermentComp, HitData);
}

bool ADismemberableActor::CanBeDismembered_Implementation() const
{
    if (DismembermentComp)
        return IDismemberable::Execute_CanBeDismembered(DismembermentComp);
    return false;
}