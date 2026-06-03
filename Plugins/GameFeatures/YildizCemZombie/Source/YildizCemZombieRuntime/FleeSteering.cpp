#include "FleeSteering.h"
#include "GameFramework/Pawn.h"

UFleeSteering::UFleeSteering()
{
	PanicDistance = 1000.0f;
	MaxFleeDistance = 2000.0f;
	MaxSpeed = 600.0f;
	Weight = 1.5f; 
}

void UFleeSteering::SetThreatLocation(const FVector& Threat)
{
	ThreatLocation = Threat;
	ThreatActor = nullptr;
}

void UFleeSteering::SetThreatActor(AActor* Threat)
{
	ThreatActor = Threat;
}

FVector UFleeSteering::CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity)
{
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	FVector Threat = ThreatLocation;
	if (ThreatActor)
	{
		Threat = ThreatActor->GetActorLocation();
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector FromThreat = PawnLocation - Threat;
	const float Distance = FromThreat.Size();

	if (Distance > MaxFleeDistance)
	{
		return FVector::ZeroVector;
	}
	if (Distance > PanicDistance)
	{
		return FVector::ZeroVector;
	}
	
	FVector DesiredVelocity = FromThreat.GetSafeNormal() * MaxSpeed;

	if (Distance < PanicDistance)
	{
		const float PanicMultiplier = 1.0f + (1.0f - (Distance / PanicDistance));
		DesiredVelocity *= PanicMultiplier;
	}

	FVector SteeringForce = DesiredVelocity - CurrentVelocity;
	SteeringForce = Truncate(SteeringForce, MaxForce);

	return SteeringForce * Weight;
}