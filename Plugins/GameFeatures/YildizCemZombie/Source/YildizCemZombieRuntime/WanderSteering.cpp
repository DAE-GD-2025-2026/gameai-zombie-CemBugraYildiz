#include "WanderSteering.h"
#include "GameFramework/Pawn.h"

UWanderSteering::UWanderSteering()
{
	WanderRadius = 200.0f;
	WanderDistance = 400.0f;
	WanderJitter = 100.0f;
	MaxSpeed = 400.0f; 
	
	WanderTarget = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		0.0f
	).GetSafeNormal() * WanderRadius;
}

FVector UWanderSteering::CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity)
{
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	FVector JitterThisFrame = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		0.0f
	) * WanderJitter;

	WanderTarget += JitterThisFrame;

	WanderTarget = WanderTarget.GetSafeNormal() * WanderRadius;

	const FVector Forward = Pawn->GetActorForwardVector();
	const FVector CircleCenter = Forward * WanderDistance;

	const FVector TargetWorld = Pawn->GetActorLocation() + CircleCenter + WanderTarget;

	const FVector ToTarget = (TargetWorld - Pawn->GetActorLocation()).GetSafeNormal();
	FVector DesiredVelocity = ToTarget * MaxSpeed;

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	SteeringForce = Truncate(SteeringForce, MaxForce);

	return SteeringForce * Weight;
}