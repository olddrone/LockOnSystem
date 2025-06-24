// Fill out your copyright notice in the Description page of Project Settings.

#include "LockOnComponent.h"
#include "Engine/OverlapResult.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);   
	
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetBPClass(TEXT("/LockOnSystem/WBP_LockOn.WBP_LockOn_C"));
	if (WidgetBPClass.Succeeded())
		LockOnWidgetClass = WidgetBPClass.Class;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<APawn>(GetOwner());
	Controller = Cast<APlayerController>(Cast<APawn>(Owner)->GetController());
	if (IsValid(Controller))
		Camera = Controller->PlayerCameraManager;
	Params.AddIgnoredActor(Owner);
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsLockOn || !IsValid(TargetActor) || !Owner->IsLocallyControlled())
		return;

	SetRotationOnTarget(GetControlRotationOnTarget());
	
	if (GetDistToTarget() > MaxLockOnDist)
		LockOff();

	if (CheckTargetOutOfSight() && bCheckLOS) {
		if (LOSCooldown > 0.f) {
			bCheckLOS = false;
			GetWorld()->GetTimerManager().SetTimer(LOSTimerHandle, FTimerDelegate::CreateLambda([&]() {
					bCheckLOS = true;
					if (CheckTargetOutOfSight())
						LockOff();
				}), LOSCooldown, false);
		}
		else
			LockOff();
	}
}

void ULockOnComponent::LockOn()
{
	HitActors.Reset();
	
	SweepPhase();	// 1. ���� �ִ� �������� Capsule�� ����(SweepMulti)
	LinePhase();	// 2. Line���� 2�� ����(��ֹ�) 

	if (HitActors.Num() > 0) 
		SetTarget(FindNearestActor());
}

void ULockOnComponent::LockOff()
{
	HitActors.Reset();

	if (IsValid(TargetActor)) {
		TargetActor = nullptr;
		bIsLockOn = false;

		ControlRotation(false);
		if (IsValid(Controller))
			Controller->ResetIgnoreLookInput();

		if (IsValid(LockOnWidgetComp))
			LockOnWidgetComp->DestroyComponent();
	}
}

void ULockOnComponent::SweepPhase()
{
	const FVector_NetQuantize CamForward = Camera->GetActorForwardVector();
	const float HalfHeight = MaxLockOnDist * 0.5f;
	const FVector_NetQuantize Center = Owner->GetActorLocation() + CamForward * HalfHeight;
	const FQuat Rotation = FRotationMatrix::MakeFromZ(CamForward).ToQuat();

	TArray<FOverlapResult> OutHits;
	// �⺻ ���� Ignore, ������ ���͸� Block����
	const bool bResult = GetWorld()->OverlapMultiByChannel(OutHits, Center, Rotation, 
		ECC_GameTraceChannel1, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);

	if (bDrawDebug)
		DrawDebugCapsule(GetWorld(), Center, HalfHeight, Radius, Rotation, (bResult) ? FColor::Green : FColor::Orange, false, 3.f);

	for (const FOverlapResult& Hit : OutHits) {
		if (AActor* HitActor = Hit.GetActor())
			HitActors.Add(HitActor);
	}
}

void ULockOnComponent::LinePhase()
{
	TArray<AActor*> ToRemove;

	for (AActor* Actor : HitActors) {
		const FVector_NetQuantize Start = Owner->GetActorLocation();
		const FVector_NetQuantize End = Actor->GetActorLocation();

		FHitResult HitResult;
		// �⺻ ���� Block, 
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_GameTraceChannel2, Params);
		if (HitResult.GetActor() != Actor)
			ToRemove.Add(Actor);
		else
			DrawDebugLine(GetWorld(), Start, HitResult.ImpactPoint, FColor::Blue, false, 3.f);
	}

	for (AActor* Actor : ToRemove)
		HitActors.Remove(Actor);
}

AActor* ULockOnComponent::FindNearestActor()
{
	AActor* RetActor = nullptr;
	float MinDist = TNumericLimits<float>::Max();
	// 1. ȭ�� �߾� ��ġ
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter(ViewportSize/2);
	
	for (AActor* Actor : HitActors) {
		// 2. ������ ȭ�� �� ��ġ(Proj)
		FVector2D ProjLocation;
		if (Controller->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ProjLocation)) {
			// �߾Ӱ� ���� ����� ���� ����
			float Dist = FVector2D::Distance(ViewportCenter, ProjLocation);
			if (Dist < MinDist) {
				RetActor = Actor;
				MinDist = Dist;
			}
		}
	}
	return RetActor;
}

FRotator ULockOnComponent::GetControlRotationOnTarget() const
{
	if (!IsValid(Controller))
		return FRotator::ZeroRotator;

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FVector_NetQuantize OwnerLocation = Owner->GetActorLocation();
	const FVector_NetQuantize TargetLocation = TargetActor->GetActorLocation();

	const FRotator LookRotation = FRotationMatrix::MakeFromX(TargetLocation - OwnerLocation).Rotator();
	float Pitch = LookRotation.Pitch;
	FRotator TargetRotation;
	
	if (bEnableDistToTarget) {
		const float DistToTarget = GetDistToTarget();
		const float PitchInRange = (DistToTarget * PitchDistValue + PitchDistOffset) * -1.0f;
		const float PitchOffset = FMath::Clamp(PitchInRange, PitchAreaMin, PitchAreaMax);

		Pitch += PitchOffset;
		TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}
	else {
		TargetRotation = (bIgnoreLookInput) ? FRotator(Pitch, LookRotation.Yaw, LookRotation.Roll) 
			: FRotator(ControlRotation.Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}

	return FMath::RInterpTo(ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotateInterpSpeed);
}

bool ULockOnComponent::CheckTargetOutOfSight() const
{
	if (!IsValid(Owner) || !IsValid(TargetActor))
		return true;

	const FVector_NetQuantize Start= TargetActor->GetActorLocation();
	const FVector_NetQuantize End = Owner->GetActorLocation();

	// Ÿ�� -> ����(�÷��̾�), �ݴ�� ����
	// Ÿ�� --- (�ٸ� �÷��̾�) --- (����) ���µ� ���� �۵��� ���� Multi
	TArray<FHitResult> HitResults;
	// Ÿ���� Visibility ä�� Ignore
	GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility);
	
	// �÷��̾��� Visibility ä���� overlap���� �ؾ� ������ �ʰ� �հ� ��
	for (const FHitResult& Hit : HitResults) {
		if (Hit.GetActor() == Owner)
			return false;
	}

	return true;
}

void ULockOnComponent::AttachWidgetToTarget()
{
	if (!IsValid(LockOnWidgetClass) || !IsValid(TargetActor))
		return;

	LockOnWidgetComp = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName(TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn")));
	LockOnWidgetComp->SetWidgetClass(LockOnWidgetClass);

	UMeshComponent* MeshComp = TargetActor->FindComponentByClass<UMeshComponent>();
	USceneComponent* ParentComp = (IsValid(MeshComp)) ? MeshComp : TargetActor->GetRootComponent();

	LockOnWidgetComp->SetOwnerPlayer(Controller->GetLocalPlayer());
	LockOnWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnWidgetComp->SetupAttachment(ParentComp, LockOnSocketName);
	LockOnWidgetComp->SetDrawSize(FVector2D(WidgetSize));
	LockOnWidgetComp->SetVisibility(true);
	LockOnWidgetComp->RegisterComponent();
}

void ULockOnComponent::TargetWithAxisInput(const float AxisValue)
{
	if (!IsValid(TargetActor) || !bIsLockOn || !SwitchTarget(AxisValue) || !bEnableChange)
		return;

	bEnableChange = false;
	FTimerHandle ChangeHandle;
	GetWorld()->GetTimerManager().SetTimer(ChangeHandle, FTimerDelegate::CreateLambda([&]() {
		bEnableChange = true;
		}), ChangeCooldown, false);

	const int32 Dir = (AxisValue > 0.f) ? +1 : -1;
	const float Offset = 400.f;
	const FVector_NetQuantize Center = TargetActor->GetActorLocation() + (Camera->GetActorRightVector() * Dir * Offset);

	FCollisionQueryParams ChangeParams;
	ChangeParams.AddIgnoredActor(TargetActor);
	ChangeParams.AddIgnoredActor(Owner);
	TArray<FOverlapResult> OutHits;
	const bool bResult = GetWorld()->OverlapMultiByChannel(OutHits, Center, 
		FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(ChangeRadius), ChangeParams);

	if (bDrawDebug)
		DrawDebugSphere(GetWorld(), Center, ChangeRadius, 20, (bResult) ? FColor::Green : FColor::Orange, false, 3.f);

	AActor* ChangeTarget = nullptr;
	float MinDist = TNumericLimits<float>::Max();
	for (const FOverlapResult& Hit : OutHits) {
		if (AActor* HitActor = Hit.GetActor()) {
			const float Dist = FVector::Distance(TargetActor->GetActorLocation(), HitActor->GetActorLocation());
			if (Dist < MinDist) {
				ChangeTarget = HitActor;
				MinDist = Dist;
			}
		}
	}

	if (!ChangeTarget)
		return;

	LockOff();
	SetTarget(ChangeTarget);
}

void ULockOnComponent::SetTarget(AActor* InTargetActor)
{
	TargetActor = InTargetActor;
	bIsLockOn = true;

	ControlRotation(true);
	if (Controller && bIgnoreLookInput)
		Controller->SetIgnoreLookInput(true);
	AttachWidgetToTarget();
}

void ULockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ULockOnComponent, bControlRotation, COND_SkipReplay)
}

void ULockOnComponent::ControlRotation(const bool bInControlRotation)
{
	if (!IsValid(Owner) || !Owner->IsLocallyControlled())
		return;

	ServerControlRotation(bInControlRotation);
}

void ULockOnComponent::ServerControlRotation_Implementation(const bool bInControlRotation)
{
	bControlRotation = bInControlRotation;
	OnRep_ControlRotation();
}

// ���� ���� �� OnRep�� BeginPlay���� ���� ����� �� ����
void ULockOnComponent::OnRep_ControlRotation()
{
	if (!IsValid(Owner))
		return;

	Owner->bUseControllerRotationYaw = bControlRotation;
	UCharacterMovementComponent* MovementComp = Cast<UCharacterMovementComponent>(Owner->GetMovementComponent());
	if (IsValid(MovementComp))
		MovementComp->bOrientRotationToMovement = !bControlRotation;
}
