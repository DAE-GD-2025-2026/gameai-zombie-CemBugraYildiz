#include "SteeringBehavior.h"
#include "GameFramework/Pawn.h"

USteeringBehavior::USteeringBehavior()
{
	Weight = 1.0f;
	MaxSpeed = 600.0f;
	MaxForce = 300.0f;
}

FVector USteeringBehavior::CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity)
{
	return FVector::ZeroVector;
}

FVector USteeringBehavior::Truncate(const FVector& Vector, float MaxLength) const
{
	if (Vector.Size() > MaxLength)
	{
		return Vector.GetSafeNormal() * MaxLength;
	}
	return Vector;
}