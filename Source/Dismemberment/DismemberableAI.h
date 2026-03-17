#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Dismemberable.h"
#include "DismembermentTypes.h"
#include "SkeletalMeshDismembermentComp.h"
#include "DismemberableAI.generated.h"

UCLASS()
class DISMEMBERMENT_API ADismemberableAI : public ACharacter, public IDismemberable
{
    GENERATED_BODY()

public:
    ADismemberableAI();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dismemberment")
        USkeletalMeshDismembermentComp* DismembermentComp;

    virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;
    virtual bool CanBeDismembered_Implementation() const override;

protected:
    virtual void BeginPlay() override;
};