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

    UPROPERTY(BlueprintReadWrite, Category = "Dismemberment")
        FVector BladeDirection;

    UPROPERTY(BlueprintReadWrite, Category = "Dismemberment")
        int32 SwingID = -1;

    FDismembermentHitData()
    {
        BladeDirection = FVector::ZeroVector;
        SwingID = -1;
    }
};