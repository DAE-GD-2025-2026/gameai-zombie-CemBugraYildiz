#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasWeapon.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTDecorator_HasWeapon : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasWeapon();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	int32 MinAmmoRequired = 3;
};