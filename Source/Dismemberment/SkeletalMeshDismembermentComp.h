#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "SkeletalMeshDismembermentComp.generated.h"


//valid severance point on a skelly, create for each severance point/bone
//chunk actor is what is spawned on severance
USTRUCT(BlueprintType)
struct FSeverancePointData
{
	GENERATED_BODY()

	// The bone at which detachment occurs when this point is the nearest match 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
	FName BoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
		TSubclassOf<AActor> ChunkActorClass = nullptr;


	//if true, remaining skeleton is ragdolled and no more movement for it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
	bool bIsLethal = false;
};


//on hit, wlak up heirarchy to find a valid severance point
//bones below it are hidden
//actor is spawned
//this is preset with the default bone names
UCLASS(ClassGroup = (Dismemberment), meta = (BlueprintSpawnableComponent))
class DISMEMBERMENT_API USkeletalMeshDismembermentComp : public UActorComponent, public IDismemberable
{
	GENERATED_BODY()

public:

	USkeletalMeshDismembermentComp();

	//valid points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dismemberment")
		TArray<FSeverancePointData> SeverancePoints;

	virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;
	virtual bool CanBeDismembered_Implementation() const override;

private:

	TSet<FName> DetachedBones;

	//walks up bone heirarchy from hit bone until valid point is found
	//returns NAME_none or no match
	FName FindNearestSeverancePoint(USkeletalMeshComponent* SkelMesh, FName HitBone) const;

	//hide severance bone and its children
	void HideBoneChain(USkeletalMeshComponent* SkelMesh, FName RootBone) const;

	
	bool IsBoneChildOf(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, int32 AncestorIndex) const;

	void SpawnDetachedChunk(USkeletalMeshComponent* SkelMesh, FName SeveranceBone, const FSeverancePointData& SeveranceData) const;

	
	void HandleLethalSeverance(USkeletalMeshComponent* SkelMesh) const;
};