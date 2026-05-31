#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SteeringBehavior.generated.h"

UCLASS(Abstract, Blueprintable)
class YILDIZCEMZOMBIERUNTIME_API USteeringBehavior : public UObject
{
	GENERATED_BODY()

public:
	USteeringBehavior();

	UFUNCTION(BlueprintCallable, Category = "Steering")
	virtual FVector CalculateSteering(APawn* Pawn, const FVector& CurrentVelocity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxForce = 300.0f;

protected:
	FVector Truncate(const FVector& Vector, float MaxLength) const;
};