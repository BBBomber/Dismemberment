#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DismembermentTypes.h"
#include "WeaponActor.generated.h"


//Sword that does per frame sweep only during the animation
//Attaches to chars hand socket

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
	void PerformBladeTrace();
	void DispatchHit(const FHitResult& HitResult, const FVector& BladeDirection);
};