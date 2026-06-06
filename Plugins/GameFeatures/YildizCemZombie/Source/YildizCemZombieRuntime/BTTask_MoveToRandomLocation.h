#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToRandomLocation.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_MoveToRandomLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToRandomLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SearchRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector TargetLocationKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float Cooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector LastMoveTimeKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering")
	bool bUseWanderSteering = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering")
	float WanderDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Exploration")
	bool bUseClusterExploration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Exploration")
	bool bAvoidRecentlyVisited = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Exploration")
	float VisitedLocationAvoidanceRadius = 1500.0f;

private:
	float WanderStartTime = 0.0f;
	void ExploreUsingWander(UBehaviorTreeComponent& OwnerComp);
	
	FVector FindClusterBasedExploration(UBehaviorTreeComponent& OwnerComp, const FVector& CurrentLocation);
	FVector FindLeastVisitedDirection(UBehaviorTreeComponent& OwnerComp, const FVector& CurrentLocation);
	bool IsLocationSuitableForExploration(class UExplorationMemory* Memory, const FVector& Location);
};