#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckAmmo.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTDecorator_CheckAmmo : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckAmmo();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Ammo")
	int32 MinAmmo = 5;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AmmoKey;
};