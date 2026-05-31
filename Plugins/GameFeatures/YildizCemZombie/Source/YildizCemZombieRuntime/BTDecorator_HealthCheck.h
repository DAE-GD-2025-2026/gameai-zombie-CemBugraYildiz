#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HealthCheck.generated.h"

UENUM(BlueprintType)
enum class EHealthComparison : uint8
{
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual
};

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTDecorator_HealthCheck : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HealthCheck();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	float HealthThreshold = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Condition")
	EHealthComparison ComparisonType = EHealthComparison::Less;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HealthKey;
};