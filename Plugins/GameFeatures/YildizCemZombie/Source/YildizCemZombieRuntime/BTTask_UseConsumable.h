#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Items/BaseItem.h"
#include "BTTask_UseConsumable.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_UseConsumable : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_UseConsumable();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Item")
	EItemType ItemTypeToUse = EItemType::Medkit;
};