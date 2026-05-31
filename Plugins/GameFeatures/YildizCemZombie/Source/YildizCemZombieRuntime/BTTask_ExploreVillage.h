#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExploreVillage.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_ExploreVillage : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExploreVillage();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ExplorationRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float HouseVisitDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bMarkAsExploredWhenDone = true;

private:
	bool ExploreCluster(UBehaviorTreeComponent& OwnerComp, int32 ClusterIndex, const FVector& CurrentLocation);
};