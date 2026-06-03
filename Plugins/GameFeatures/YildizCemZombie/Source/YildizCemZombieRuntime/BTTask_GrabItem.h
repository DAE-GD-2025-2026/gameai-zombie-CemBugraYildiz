#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_GrabItem.generated.h"

struct FGrabItemTaskMemory
{
	TWeakObjectPtr<AActor> CachedTarget;
	float TaskStartTime; 
};

class UInventoryComponent;
class ABaseItem;

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_GrabItem : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GrabItem();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override 
	{ 
		return sizeof(FGrabItemTaskMemory); 
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector TargetItemKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float GrabDistance = 300.0f;
	
private:
	int32 FindEmptySlot(UInventoryComponent* InventoryComp) const;
};