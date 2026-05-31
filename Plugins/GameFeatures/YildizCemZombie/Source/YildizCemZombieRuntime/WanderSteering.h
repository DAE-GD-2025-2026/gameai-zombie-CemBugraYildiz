#pragma once

#include "CoreMinimal.h"
#include "SteeringBehavior.h"
#include "WanderSteering.generated.h"

UCLASS(Blueprintable, ClassGroup = (Steering))
class YILDIZCEMZOMBIERUNTIME_API UWanderSteering : public USteeringBehavior
{
	GENERATED_BODY()

public:
	UWanderSteering();

	virtual FVector CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Wander")
	float WanderRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Wander")
	float WanderDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Wander")
	float WanderJitter = 100.0f;

private:
	FVector WanderTarget;
};