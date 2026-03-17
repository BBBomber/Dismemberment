#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "WeaponActor.h"
#include "DismembermentCharacter.generated.h"

UCLASS(config = Game)
class ADismembermentCharacter : public ACharacter
{
    GENERATED_BODY()

        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
        class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
        class UCameraComponent* FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
        class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
        class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
        class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
        class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
        class UInputAction* AttackAction;

    FTimerHandle AttackEndTimer;

public:
    ADismembermentCharacter();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        TSubclassOf<AWeaponActor> WeaponClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
        AWeaponActor* CurrentWeapon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
        class UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, Category = "Weapon")
        FVector WeaponLocationOffset;

    UPROPERTY(EditAnywhere, Category = "Weapon")
        FRotator WeaponRotationOffset;

protected:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Attack(const FInputActionValue& Value);
    void SpawnAndEquipWeapon();
    void ReEnableMovement();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;

public:
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};