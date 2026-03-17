#include "DismemberableAI.h"

ADismemberableAI::ADismemberableAI()
{
    PrimaryActorTick.bCanEverTick = false;
    DismembermentComp = CreateDefaultSubobject<USkeletalMeshDismembermentComp>(TEXT("DismembermentComp"));
}

void ADismemberableAI::BeginPlay()
{
    Super::BeginPlay();

    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void ADismemberableAI::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (DismembermentComp)
        IDismemberable::Execute_ProcessHit(DismembermentComp, HitData);
}

bool ADismemberableAI::CanBeDismembered_Implementation() const
{
    if (DismembermentComp)
        return IDismemberable::Execute_CanBeDismembered(DismembermentComp);
    return false;
}