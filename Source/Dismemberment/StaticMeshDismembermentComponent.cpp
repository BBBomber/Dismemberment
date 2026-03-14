// Fill out your copyright notice in the Description page of Project Settings.

#include "StaticMeshDismembermentComponent.h"
#include "DismembermentTypes.h"
#include "Components/StaticMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "StaticMeshDismembermentComponent.h"

// Sets default values for this component's properties
UStaticMeshDismembermentComponent::UStaticMeshDismembermentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;


}

void UStaticMeshDismembermentComponent::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
	if (!CanBeDismembered_Implementation()) {
		FadeAndDestroy(); //since depth would be 0
		return;
	}

	if (!bIsProceduralMesh) {
		ConvertToProceduralMesh();
	}

	if (!ProceduralMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ProceduralMesh is null after conversion on %s"), *GetOwner()->GetName());
		return;
	}

	FVector PlaneNormal;
	FVector PlanePos;

	ComputeSlicePlane(HitData, PlaneNormal, PlanePos);
	ExecuteSlice(PlanePos, PlaneNormal);


}

bool UStaticMeshDismembermentComponent::CanBeDismembered_Implementation() const
{
	return SliceDepth > 0;
}

void UStaticMeshDismembermentComponent::ConvertToProceduralMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UStaticMeshComponent* StaticMeshComp = Owner->FindComponentByClass<UStaticMeshComponent>();
	if (!StaticMeshComp)
	{
		UE_LOG(LogTemp, Error, TEXT("StaticMeshDismembermentComponent: No StaticMeshComponent found on actor %s"), *GetOwner()->GetName());
		return;
	}

	
	ProceduralMesh = NewObject<UProceduralMeshComponent>(Owner, TEXT("ProceduralMesh"));
	ProceduralMesh->RegisterComponent();
	ProceduralMesh->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::KeepWorldTransform
	);
	ProceduralMesh->SetWorldTransform(StaticMeshComp->GetComponentTransform());

	//copy the static mesh geometry over, this creates the coll geometry as well
	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(StaticMeshComp,0,ProceduralMesh,true);

	// Hide the original static mesh
	StaticMeshComp->SetVisibility(false);
	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bIsProceduralMesh = true;
}

void UStaticMeshDismembermentComponent::ComputeSlicePlane(const FDismembermentHitData& HitData, FVector& OutPlaneNormal, FVector& OutPlanePosition) const
{
	OutPlanePosition = HitData.HitResult.ImpactPoint; //where the weapon first touches
	OutPlaneNormal = FVector::CrossProduct(HitData.BladeDirection, HitData.HitResult.ImpactNormal).GetSafeNormal();

	if (OutPlaneNormal.IsNearlyZero()) {
		OutPlaneNormal = HitData.HitResult.ImpactNormal;
	}
}

void UStaticMeshDismembermentComponent::ExecuteSlice(const FVector& PlanePosition, const FVector& PlaneNormal)
{
	UProceduralMeshComponent* OtherHalfMesh = nullptr;

	UKismetProceduralMeshLibrary::SliceProceduralMesh(ProceduralMesh, PlanePosition, PlaneNormal, true, OtherHalfMesh, EProcMeshSliceCapOption::CreateNewSectionForCap, CapMaterial );

	ProceduralMesh->SetSimulatePhysics(true);

	// Spawn the other half 
	if (OtherHalfMesh)
	{
		SpawnSlicedPiece(OtherHalfMesh);
	}

	SliceDepth--;
}

void UStaticMeshDismembermentComponent::SpawnSlicedPiece(UProceduralMeshComponent* OtherHalfMesh)
{
	if (!OtherHalfMesh)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	AActor* PieceActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), OtherHalfMesh->GetComponentLocation(), OtherHalfMesh->GetComponentRotation(), SpawnParams);

	if (!PieceActor)
	{
		return;
	}

	// Detach the other half mesh from the original actor and move it to the new one
	// Rename transfers UObject ownership to the new actor
	OtherHalfMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	OtherHalfMesh->Rename(nullptr, PieceActor);
	PieceActor->SetRootComponent(OtherHalfMesh);
	OtherHalfMesh->RegisterComponent();

	// Enable physics on this half as well
	OtherHalfMesh->SetSimulatePhysics(true);

	// Add a dismemberment component to the new piece so it can be sliced again
	// Pass down SliceDepth - 1 so depth is correctly tracked across pieces
	UStaticMeshDismembermentComponent* NewDismemberment = NewObject<UStaticMeshDismembermentComponent>(PieceActor);
	NewDismemberment->SliceDepth = SliceDepth - 1;
	NewDismemberment->CapMaterial = CapMaterial;
	NewDismemberment->ProceduralMesh = OtherHalfMesh;
	NewDismemberment->bIsProceduralMesh = true;
	NewDismemberment->RegisterComponent();
}

void UStaticMeshDismembermentComponent::FadeAndDestroy()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	//no fade for now 
	Owner->Destroy();
}






