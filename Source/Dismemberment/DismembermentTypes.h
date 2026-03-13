#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "DismembermentTypes.generated.h"

//payload from weapon to whatever gets cut


USTRUCT(BlueprintType)
struct DISMEMBERMENT_API FDismembermentHitData
{
	GENERATED_BODY()

	
	UPROPERTY(BlueprintReadWrite, Category = "Dismemberment")
	FHitResult HitResult;

	//dir at momement of contact used for slice plane
	UPROPERTY(BlueprintReadWrite, Category = "Dismemberment")
	FVector BladeDirection;

	FDismembermentHitData()
	{
		BladeDirection = FVector::ZeroVector;
		
	}
};