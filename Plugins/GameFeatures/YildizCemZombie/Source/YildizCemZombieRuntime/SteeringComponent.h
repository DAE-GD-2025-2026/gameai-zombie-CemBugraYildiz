#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SteeringComponent.generated.h"

class USteeringBehavior;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YILDIZCEMZOMBIERUNTIME_API USteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USteeringComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void AddSteeringBehavior(USteeringBehavior* Behavior);

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void RemoveSteeringBehavior(USteeringBehavior* Behavior);

	UFUNCTION(BlueprintCallable, Category = "Steering")
	void ClearAllBehaviors();

	UFUNCTION(BlueprintCallable, Category = "Steering")
	FVector CalculateTotalSteering();

	UFUNCTION(BlueprintCallable, Category = "Steering")
	int32 GetActiveBehaviorCount() const { return SteeringBehaviors.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Steering")
	bool HasActiveBehaviors() const { return SteeringBehaviors.Num() > 0; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	bool bAutoApplySteering = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float Mass = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	float MaxSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Debug")
	bool bDrawDebug = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<USteeringBehavior*> SteeringBehaviors;

	FVector CurrentVelocity;

private:
	void ApplySteeringForce(const FVector& SteeringForce, float DeltaTime);
};