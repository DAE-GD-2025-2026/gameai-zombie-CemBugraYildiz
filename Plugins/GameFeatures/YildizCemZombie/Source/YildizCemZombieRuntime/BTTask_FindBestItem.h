#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindBestItem.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_FindBestItem : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindBestItem();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector BestItemKey;
};