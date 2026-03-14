// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "DismembermentCharacter.h"
#include "ANS_WeaponTrace.generated.h"

/**
 * 
 */
UCLASS()
class DISMEMBERMENT_API UANS_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UANS_WeaponTrace();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* ANimation, float frameDeltaTime, const FAnimNotifyEventReference& EventRefreence) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	AWeaponActor* CachedWeapon; //get in begin notify from owner
	AWeaponActor* GetWeaponFromOwner(USkeletalMeshComponent* MeshComp) const;
};
