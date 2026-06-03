#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToSafeHouse.generated.h"

class AHouse;

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_MoveToSafeHouse : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToSafeHouse();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector SafeHouseKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float EnterDistance = 500.0f;

private:
	bool IsInsideHouse(APawn* Pawn, AActor* House) const;
};