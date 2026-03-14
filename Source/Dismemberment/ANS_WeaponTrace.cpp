// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_WeaponTrace.h"
#include "WeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UANS_WeaponTrace::UANS_WeaponTrace()
{
	CachedWeapon = nullptr;
}

void UANS_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	CachedWeapon = GetWeaponFromOwner(MeshComp);
	if (!CachedWeapon) {
		return;
	}

	CachedWeapon->EnableTrace();
}

void UANS_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* ANimation, float frameDeltaTime, const FAnimNotifyEventReference& EventRefreence)
{
	if (!CachedWeapon) {
		return;
	}

	CachedWeapon->TickTrace();
}

void UANS_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CachedWeapon) {
		return;
	}

	CachedWeapon->DisableTrace();

}

AWeaponActor* UANS_WeaponTrace::GetWeaponFromOwner(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) {
		return nullptr;
	}

	ADismembermentCharacter* Character = Cast<ADismembermentCharacter>(MeshComp->GetOwner());
	if (!Character) {
		return nullptr;
	}

	return Character->CurrentWeapon;
}
