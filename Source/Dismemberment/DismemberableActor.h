// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dismemberable.h"
#include "StaticMeshDismembermentComponent.h"
#include "DismemberableActor.generated.h"

UCLASS()
class DISMEMBERMENT_API ADismemberableActor : public AActor, public IDismemberable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADismemberableActor();


	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dismemberment")
	UStaticMeshDismembermentComponent* DismembermentComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dismemberment")
		UProceduralMeshComponent* ProceduralMesh;

protected:
	virtual void BeginPlay() override;

public:	
	

	virtual void ProcessHit_Implementation(const FDismembermentHitData& HitData) override;
	virtual bool CanBeDismembered_Implementation() const override;

};
