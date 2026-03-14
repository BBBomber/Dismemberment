#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshDismembermentComponent.generated.h"



//makes the actor with a static mesh dismemberable
//can be sliced into new actors until slice depth hits 0
UCLASS(ClassGroup = (Dismemberment), meta = (BlueprintSpawnableComponent))
class DISMEMBERMENT_API UStaticMeshDismembermentComponent : public UActorComponent, public IDismemberable
{
	GENERATED_BODY()

public:

	UStaticMeshDismembermentComponent();


	//when 0, CanBeDismembered will return false and pieces will fade
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
		int32 SliceDepth = 2;

	//for the new plane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
		UMaterialInterface* CapMaterial = nullptr; 

	// implement interface
	virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;
	virtual bool CanBeDismembered_Implementation() const override;

private:

	//null unitl first hit
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh = nullptr;

	//true if static mesh has been converted
	bool bIsProceduralMesh = false;

	//conver static to procedural mesh
	//hide original and copy geometry over
	//call on first hit
	void ConvertToProceduralMesh();

	void ComputeSlicePlane(const FDismembermentHitData& HitData, FVector& OutPlaneNormal, FVector& OutPlanePosition) const;

	
	void ExecuteSlice(const FVector& PlanePosition, const FVector& PlaneNormal);

	//takes the other sliced part and moves it into a new actor with the same comp
	//start with depth-1
	void SpawnSlicedPiece(UProceduralMeshComponent* OtherHalfMesh);

	//when depth is 0 and another hit is registered.
	void FadeAndDestroy();
};