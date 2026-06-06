#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExploreVillage.generated.h"

struct FExploreTaskMemory
{
	FVector  TargetLocation;
	float    ArrivalTime;    
	bool     bJustArrived;   
	float    NavStartTime;
	AActor*  TargetHouseActor;

	FExploreTaskMemory()
		: TargetLocation(FVector::ZeroVector), ArrivalTime(-1.0f), bJustArrived(false),
		  NavStartTime(-1.0f), TargetHouseActor(nullptr) {}
};

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_ExploreVillage : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExploreVillage();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FExploreTaskMemory); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ExplorationRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float HouseVisitDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bMarkAsExploredWhenDone = true;

private:
	bool ExploreCluster(UBehaviorTreeComponent& OwnerComp, int32 ClusterIndex,
					const FVector& CurrentLocation, FVector& OutTargetLocation, AActor*& OutTargetHouse);
};