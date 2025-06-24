// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class UUserWidget;
class UWidgetComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LOCKONSYSTEM_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULockOnComponent();

	void DoTargeting() { (!bIsLockOn) ? LockOn() : LockOff(); }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void TargetWithAxisInput(const float AxisValue);

protected:
	virtual void BeginPlay() override;

private:
	void LockOn();
	void LockOff();

	void SweepPhase();
	void LinePhase();
	AActor* FindNearestActor();

	FRotator GetControlRotationOnTarget() const;
	FORCEINLINE void SetRotationOnTarget(const FRotator& ControllerRotation) { Controller->SetControlRotation(ControllerRotation); }
	
	FORCEINLINE float GetDistToTarget() const { return Owner->GetDistanceTo(TargetActor); }
	FORCEINLINE bool SwitchTarget(const float AxisValue) const { return FMath::Abs(AxisValue) > RotateThreshold; }

	bool CheckTargetOutOfSight() const;


	void AttachWidgetToTarget();

	void SetTarget(AActor* InTargetActor);

private:
	UPROPERTY()
	TObjectPtr<APawn> Owner;

	UPROPERTY()
	TObjectPtr<APlayerController> Controller;

	UPROPERTY()
	TObjectPtr<APlayerCameraManager> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float MaxLockOnDist = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float Radius = 300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	bool bIsLockOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	bool bDrawDebug = true;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

	FCollisionQueryParams Params;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	bool bEnableDistToTarget = false;


// 타겟 액터에 대한 플레이어 로테이션 관련 변수들
	UPROPERTY(EditAnywhere, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float PitchDistValue = -0.2f;

	UPROPERTY(EditAnywhere, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float PitchDistOffset = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float PitchAreaMin = -50.f;

	UPROPERTY(EditAnywhere, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float PitchAreaMax = -20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	bool bIgnoreLookInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float RotateInterpSpeed = 9.f;


// 틱에서 타겟과 오너 사이에 장애물 검사 관련
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	bool bCheckLOS = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float LOSCooldown = 1.5f;

	FTimerHandle LOSTimerHandle;

// 위젯 관련
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> LockOnWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> LockOnWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	FName LockOnSocketName = FName("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float WidgetSize = 32.0f;

// 락온 스위치 관련 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float ChangeRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float RotateThreshold = 0.85f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info", meta = (AllowPrivateAccess = "true"))
	bool bEnableChange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setting", meta = (AllowPrivateAccess = "true"))
	float ChangeCooldown = 0.5f;


	// 시도
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ControlRotation();

private:
	UFUNCTION()
	void ControlRotation(const bool bInControlRotation);

	UFUNCTION(Server, Reliable)
	void ServerControlRotation(const bool bInControlRotation);

private:
	UPROPERTY(ReplicatedUsing = OnRep_ControlRotation, Transient, VisibleInstanceOnly)
	bool bControlRotation = false;
};
