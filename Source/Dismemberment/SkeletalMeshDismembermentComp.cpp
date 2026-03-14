#include "SkeletalMeshDismembermentComp.h"
#include "DismembermentTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
		Point.ChunkActorClass = nullptr;
		SeverancePoints.Add(Point);
	}
}


void USkeletalMeshDismembermentComp::ProcessHit_Implementation(const FDismembermentHitData& HitData)
{
	if (!CanBeDismembered_Implementation())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USkeletalMeshComponent* SkelMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!SkelMesh)
	{
		UE_LOG(LogTemp, Error, TEXT(" No SkeletalMeshComponent found on actor %s"), *Owner->GetName());
		return;
	}

	//populated on contact with physics asset
	FName HitBone = HitData.HitResult.BoneName;
	if (HitBone == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("No bone name in HitResult on actor %s. No physics asset assigned?"), *Owner->GetName());
		return;
	}


	FName SeveranceBone = FindNearestSeverancePoint(SkelMesh, HitBone);
	if (SeveranceBone == NAME_None)
	{
		return;
	}

	if (DetachedBones.Contains(SeveranceBone))
	{
		return;
	}

	const FSeverancePointData* SeveranceData = nullptr;
	for (const FSeverancePointData& Point : SeverancePoints)
	{
		if (Point.BoneName == SeveranceBone)
		{
			SeveranceData = &Point;
			break;
		}
	}

	if (!SeveranceData)
	{
		return;
	}


	HideBoneChain(SkelMesh, SeveranceBone);

	
	SpawnDetachedChunk(SkelMesh, SeveranceBone, *SeveranceData);

	
	if (SeveranceData->bIsLethal)
	{
		HandleLethalSeverance(SkelMesh);
	}

	
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
				return CurrentBone;
			}
		}

		// Move up to the parent bone and check again
		CurrentBone = SkelMesh->GetParentBone(CurrentBone);
	}

	
	return NAME_None;
}

void USkeletalMeshDismembermentComp::HideBoneChain(USkeletalMeshComponent* SkelMesh, FName RootBone) const
{
	if (!SkelMesh)
	{
		return;
	}

	USkeletalMesh* SkeletalMeshAsset = SkelMesh->GetSkeletalMeshAsset();
	if (!SkeletalMeshAsset)
	{
		return;
	}

	//read the default skelton
	const FReferenceSkeleton& RefSkeleton = SkeletalMeshAsset->GetRefSkeleton();
	const int32 RootBoneIndex = RefSkeleton.FindBoneIndex(RootBone);

	if (RootBoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT(" Bone %s not found on skeleton"), *RootBone.ToString());
		return;
	}

	//hide main bone before hiding children
	SkelMesh->HideBoneByName(RootBone, EPhysBodyOp::PBO_None); //so we can ragdoll later

	// hide any that are descendants
	for (int32 i = 0; i < RefSkeleton.GetNum(); i++)
	{
		if (IsBoneChildOf(RefSkeleton, i, RootBoneIndex))
		{
			FName BoneName = RefSkeleton.GetBoneName(i);
			SkelMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
		}
	}
}

bool USkeletalMeshDismembermentComp::IsBoneChildOf(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, int32 AncestorIndex) const
{
	
	// If we reach AncestorIndex the bone is a descendant — return true
	// If we reach the root without finding it — return false
	int32 CurrentIndex = RefSkeleton.GetParentIndex(BoneIndex);

	while (CurrentIndex != INDEX_NONE) //-1
	{
		if (CurrentIndex == AncestorIndex)
		{
			return true;
		}

		CurrentIndex = RefSkeleton.GetParentIndex(CurrentIndex);
	}

	return false;
}

void USkeletalMeshDismembermentComp::SpawnDetachedChunk(USkeletalMeshComponent* SkelMesh, FName SeveranceBone, const FSeverancePointData& SeveranceData) const
{
	if (!SeveranceData.ChunkActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("detachement actor not assigned"), *SeveranceBone.ToString());
		return;
	}

	
	const FTransform BoneTransform = SkelMesh->GetBoneTransform(SkelMesh->GetBoneIndex(SeveranceBone));

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();

	AActor* ChunkActor = GetWorld()->SpawnActor<AActor>(SeveranceData.ChunkActorClass,BoneTransform.GetLocation(),BoneTransform.GetRotation().Rotator(),SpawnParams);

	if (!ChunkActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn detached actor"), *SeveranceBone.ToString());
		return;
	}

	
	UPrimitiveComponent* ChunkRoot = Cast<UPrimitiveComponent>(ChunkActor->GetRootComponent());
	if (ChunkRoot)
	{
		ChunkRoot->SetSimulatePhysics(true);
	}
}

void USkeletalMeshDismembermentComp::HandleLethalSeverance(USkeletalMeshComponent* SkelMesh) const
{
	if (!SkelMesh)
	{
		return;
	}

	// Stop animation and transition to ragdoll
	SkelMesh->SetAllBodiesSimulatePhysics(true);
	SkelMesh->SetSimulatePhysics(true);

	// Disable character movement 
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character)
	{
		Character->GetCharacterMovement()->DisableMovement();
	}
}