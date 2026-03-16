// Fill out your copyright notice in the Description page of Project Settings.


#include "DismemberableActor.h"

// Sets default values
ADismemberableActor::ADismemberableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	DismembermentComp = CreateDefaultSubobject<UStaticMeshDismembermentComponent>(TEXT("DismembermentComp"));
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    ProceduralMesh->SetupAttachment(RootComponent);

}

void ADismemberableActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("ADismemberableActor BeginPlay called on %s"), *GetName());
    UE_LOG(LogTemp, Warning, TEXT("ProceduralMesh is %s"), ProceduralMesh ? TEXT("valid") : TEXT("null"));
    DismembermentComp->SetProceduralMesh(ProceduralMesh);
    UE_LOG(LogTemp, Warning, TEXT("SetProceduralMesh called"));
}

void ADismemberableActor::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
    if (DismembermentComp)
    {
        IDismemberable::Execute_ProcessHit(DismembermentComp, HitData);
    }
}

bool ADismemberableActor::CanBeDismembered_Implementation() const
{
    if (DismembermentComp)
    {
        return IDismemberable::Execute_CanBeDismembered(DismembermentComp);
    }
    return false;
}

