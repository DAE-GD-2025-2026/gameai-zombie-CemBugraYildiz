#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FleeFromDanger.generated.h"

struct FFleeTaskMemory
{
	float TaskStartTime;
};

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_FleeFromDanger : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FleeFromDanger();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override 
	{ 
		return sizeof(FFleeTaskMemory); 
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector DangerActorKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FleeDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bPreferHousesWhenFleeing = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering")
	bool bUseSteeringBehavior = true;
	
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	AActor* FindNearestSafeHouse(UBlackboardComponent* Blackboard, const FVector& PawnLocation, AActor* Threat) const;
	void FleeUsingSteering(UBehaviorTreeComponent& OwnerComp, AActor* Threat);
};