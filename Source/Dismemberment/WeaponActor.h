#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DismembermentTypes.h"
#include "WeaponActor.generated.h"


//Sword that does per frame sweep only during the animation
//Spawned by char at start

//Tracks socket pos per frame for vel calcs
//performs trace
//Creates the hit data using the dismemberment struct
//Lets the actor know it was struck

UCLASS()
class DISMEMBERMENT_API AWeaponActor : public AActor
{
	GENERATED_BODY()

public:

	AWeaponActor();


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* BladeStaticMesh;

	//called by an anim notify during the animation 
	void EnableTrace();

	//same
	void DisableTrace();

	//also called by the notify
	void TickTrace();

private:
	bool bTraceActive;
	int32 CurrentSwingID;
	FVector LastFrameTipPosition;
	FVector LastFrameBasePosition;

	//sweep using a capsule from blade base to tip in len and from prev base to current tip
	//gets the cut plane or blade plane which is just the cros prod of the base to top dir and tip curr - tip prev dir
	//then dispaches that plus hit result
	void PerformBladeTrace();

	//check for the IDismemberable interface and calls
	void DispatchHit(const FHitResult& HitResult, const FVector& BladeDirection);
};