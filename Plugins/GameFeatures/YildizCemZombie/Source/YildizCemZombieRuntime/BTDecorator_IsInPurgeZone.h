#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInPurgeZone.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTDecorator_IsInPurgeZone : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IsInPurgeZone();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PurgeZoneKey;
};