#include "SteeringComponent.h"
#include "SteeringBehavior.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/FloatingPawnMovement.h"

USteeringComponent::USteeringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;
	
	bAutoApplySteering = false;
	Mass = 1.0f;
	MaxSpeed = 600.0f;
	bDrawDebug = false;
	
	CurrentVelocity = FVector::ZeroVector;
}

void USteeringComponent::BeginPlay()
{
	Super::BeginPlay();
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		CurrentVelocity = OwnerPawn->GetVelocity();
	}
}

void USteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoApplySteering && SteeringBehaviors.Num() > 0)
	{
		FVector TotalSteering = CalculateTotalSteering();
		ApplySteeringForce(TotalSteering, DeltaTime);
	}
}

void USteeringComponent::AddSteeringBehavior(USteeringBehavior* Behavior)
{
	if (Behavior)
	{
		SteeringBehaviors.AddUnique(Behavior);
	}
}

void USteeringComponent::RemoveSteeringBehavior(USteeringBehavior* Behavior)
{
	SteeringBehaviors.Remove(Behavior);
}

void USteeringComponent::ClearAllBehaviors()
{
	SteeringBehaviors.Empty();
	CurrentVelocity = FVector::ZeroVector;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		UFloatingPawnMovement* FPM = Cast<UFloatingPawnMovement>(OwnerPawn->GetMovementComponent());
		if (FPM) FPM->MaxSpeed = 400.0f;
	}
}

FVector USteeringComponent::CalculateTotalSteering()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return FVector::ZeroVector;
	}

	CurrentVelocity = OwnerPawn->GetVelocity();
	FVector TotalForce = FVector::ZeroVector;

	for (USteeringBehavior* Behavior : SteeringBehaviors)
	{
		if (Behavior)
		{
			FVector Force = Behavior->CalculateSteering(OwnerPawn, CurrentVelocity);
			TotalForce += Force;
			
			if (bDrawDebug)
			{
				DrawDebugLine(
					GetWorld(),
					OwnerPawn->GetActorLocation(),
					OwnerPawn->GetActorLocation() + Force * 5.0f,
					FColor::Yellow,
					false,
					0.1f,
					0,
					2.0f
				);
			}
		}
	}

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			OwnerPawn->GetActorLocation(),
			OwnerPawn->GetActorLocation() + TotalForce * 5.0f,
			FColor::Red,
			false,
			0.1f,
			0,
			5.0f
		);
	}

	return TotalForce;
}

void USteeringComponent::ApplySteeringForce(const FVector& SteeringForce, float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	FVector Acceleration = SteeringForce / Mass;
	CurrentVelocity += Acceleration * DeltaTime;
	CurrentVelocity.Z = 0.0f;
	if (CurrentVelocity.SizeSquared() > MaxSpeed * MaxSpeed)
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;

	if (!CurrentVelocity.IsNearlyZero(10.0f))
	{
		UFloatingPawnMovement* FPM = Cast<UFloatingPawnMovement>(OwnerPawn->GetMovementComponent());
		if (FPM)
			FPM->MaxSpeed = MaxSpeed;

		OwnerPawn->AddMovementInput(CurrentVelocity.GetSafeNormal(), 1.0f);

		FRotator NewRotation = CurrentVelocity.Rotation();
		NewRotation.Pitch = 0.0f;
		NewRotation.Roll = 0.0f;
		OwnerPawn->SetActorRotation(NewRotation);
	}
}