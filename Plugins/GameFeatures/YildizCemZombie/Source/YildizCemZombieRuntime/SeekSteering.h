#pragma once

#include "CoreMinimal.h"
#include "SteeringBehavior.h"
#include "SeekSteering.generated.h"

UCLASS(Blueprintable, ClassGroup = (Steering))
class YILDIZCEMZOMBIERUNTIME_API USeekSteering : public USteeringBehavior
{
	GENERATED_BODY()

public:
	USeekSteering();

	virtual FVector CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity) override;

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void SetTargetLocation(const FVector& NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void SetTargetActor(AActor* NewTarget);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	FVector TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	AActor* TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	bool bEnableArrival = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float ArrivalRadius = 500.0f;
};