#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FleeFromPurgeZone.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_FleeFromPurgeZone : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FleeFromPurgeZone();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector PurgeZoneKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MinSafeDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SearchRadius = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering")
	bool bUseSteeringBehavior = true;

private:
	void FleeUsingSteering(UBehaviorTreeComponent& OwnerComp, AActor* PurgeZone);
};