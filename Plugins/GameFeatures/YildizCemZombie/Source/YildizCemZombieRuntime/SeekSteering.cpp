#include "SeekSteering.h"
#include "GameFramework/Pawn.h"

USeekSteering::USeekSteering()
{
	bEnableArrival = true;
	ArrivalRadius = 500.0f;
	MaxSpeed = 600.0f;
}

void USeekSteering::SetTargetLocation(const FVector& NewTarget)
{
	TargetLocation = NewTarget;
	TargetActor = nullptr;
}

void USeekSteering::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

FVector USeekSteering::CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity)
{
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	FVector Target = TargetLocation;
	if (TargetActor)
	{
		Target = TargetActor->GetActorLocation();
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector ToTarget = Target - PawnLocation;
	const float Distance = ToTarget.Size();
	const FVector ToTarget2D = FVector(ToTarget.X, ToTarget.Y, 0.0f);

	if (Distance < 10.0f)
	{
		return FVector::ZeroVector;
	}

	FVector DesiredVelocity = ToTarget2D.GetSafeNormal() * MaxSpeed;

	if (bEnableArrival && Distance < ArrivalRadius)
	{
		const float SpeedMultiplier = Distance / ArrivalRadius;
		DesiredVelocity *= SpeedMultiplier;
	}

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	SteeringForce = Truncate(SteeringForce, MaxForce);

	return SteeringForce * Weight;
}