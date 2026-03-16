#include "WeaponActor.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

AWeaponActor::AWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BladeStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Blade"));
	RootComponent = BladeStaticMesh;
	BladeStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bTraceActive = false;
	LastFrameTipPosition = FVector::ZeroVector;
	LastFrameBasePosition = FVector::ZeroVector;
}

void AWeaponActor::EnableTrace()
{
	bTraceActive = true;

	//so the first tick has valid last frame data
	LastFrameTipPosition = BladeStaticMesh->GetSocketLocation(FName("BladeTip"));
	LastFrameBasePosition = BladeStaticMesh->GetSocketLocation(FName("BladeBase"));
}

void AWeaponActor::DisableTrace()
{
	bTraceActive = false;
}


//again, called from notfiy so no need to run tick on this obj
void AWeaponActor::TickTrace()
{
	if (!bTraceActive)
	{
		return;
	}

	PerformBladeTrace();
}

void AWeaponActor::PerformBladeTrace()
{
	// Get current frame socket positions
	const FVector CurrentTipPosition = BladeStaticMesh->GetSocketLocation(FName("BladeTip"));
	const FVector CurrentBasePosition = BladeStaticMesh->GetSocketLocation(FName("BladeBase"));

	//blade dir
	const FVector BladeDirection = (CurrentTipPosition - LastFrameTipPosition).GetSafeNormal();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	TArray<FHitResult> HitResults;

	// Sweep from last frame base to current tip to cover full arc, give it a bit of width for safety
	GetWorld()->SweepMultiByChannel(
		HitResults,
		LastFrameBasePosition,
		CurrentTipPosition,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeCapsule(5.f, (CurrentTipPosition - CurrentBasePosition).Size() * 0.5f),
		QueryParams
	);


	DrawDebugLine(GetWorld(), LastFrameBasePosition, CurrentTipPosition, FColor::Red, false, 0.1f);

	for (const FHitResult& Hit : HitResults)
	{
		DispatchHit(Hit, BladeDirection);
	}

	// pos for next frame
	LastFrameTipPosition = CurrentTipPosition;
	LastFrameBasePosition = CurrentBasePosition;
}

void AWeaponActor::DispatchHit(const FHitResult& HitResult, const FVector& BladeDirection)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		//null check
		return;
	}

	
	IDismemberable* Dismemberable = Cast<IDismemberable>(HitActor);
	if (!Dismemberable)
	{
		//does not own interface
		return;
	}

	if (!IDismemberable::Execute_CanBeDismembered(HitActor))
	{
		return;
	}

	FDismembermentHitData HitData;
	HitData.HitResult = HitResult;
	HitData.BladeDirection = BladeDirection;

	// send data to hit actor :)
	IDismemberable::Execute_ProcessHit(HitActor, HitData);
}
