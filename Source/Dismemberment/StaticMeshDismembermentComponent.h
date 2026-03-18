#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "ProceduralMeshComponent.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
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

	//set by owning actor during begin play
	void SetProceduralMesh(UProceduralMeshComponent* InMesh);

	//creates this comp on sliced meshes
	void InitFromSlicedMesh(UProceduralMeshComponent* Mesh, int32 Depth, UMaterialInterface* InCapMaterial, int32 SwingID);
private:

	//null unitl first hit
	UPROPERTY()
	UProceduralMeshComponent* ProceduralMesh = nullptr;

	//true if static mesh has been converted
	bool bIsProceduralMesh = false;

	TArray<TArray<FVector>> CachedConvexVertices;

	int32 LastHitSwingID = -1;

	//conver static to procedural mesh
	//hide original and copy geometry over, procedural mesh comp needs to be on the actor from start
	// copies geometry using the kismet procedural mesh lib
	// caches convex hulls from the static mesh body setup (cocave meshes will cause issues atm, the simplified coll shape will be too big)
	//call on first hit
	void ConvertToProceduralMesh();


	//gets the impact normal from hit data
	//normal is the cross prod of blade dir and impact normal, covers degen(stab) case by just using impact normal
	void ComputeSlicePlane(const FDismembermentHitData& HitData, FVector& OutPlaneNormal, FVector& OutPlanePosition) const;

	//splits and generates a cap
	//creates 2 new actors copied into a new prcedural mesh comp for further cutting
	//adds convex coll and new dismemberment comp
	void ExecuteSlice(const FVector& PlanePosition, const FVector& PlaneNormal, int32 SwingID);



	//when depth is 0 and another hit is registered.
	//for now no fade lol just destroy
	void FadeAndDestroy();
	
};