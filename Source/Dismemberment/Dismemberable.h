// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Dismemberable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UDismemberable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DISMEMBERMENT_API IDismemberable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:


	//implementor is responsible for:
		//checking if the data is enough to trigger the cut
		//and then the actual logic ofc
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dismemberment")
	void ProcessHit(const FDismembermentHitData& HitData);


	//false when depth limit is reached for static or if all bones have been removed.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dismemberment")
	bool CanBeDismembered() const;
};
