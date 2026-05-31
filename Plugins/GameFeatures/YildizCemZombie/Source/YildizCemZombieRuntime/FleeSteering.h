#pragma once

#include "CoreMinimal.h"
#include "SteeringBehavior.h"
#include "FleeSteering.generated.h"

UCLASS(Blueprintable, ClassGroup = (Steering))
class YILDIZCEMZOMBIERUNTIME_API UFleeSteering : public USteeringBehavior
{
	GENERATED_BODY()

public:
	UFleeSteering();

	virtual FVector CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity) override;

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void SetThreatLocation(const FVector& Threat);

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void SetThreatActor(AActor* Threat);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	FVector ThreatLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	AActor* ThreatActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float PanicDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxFleeDistance = 2000.0f;
};