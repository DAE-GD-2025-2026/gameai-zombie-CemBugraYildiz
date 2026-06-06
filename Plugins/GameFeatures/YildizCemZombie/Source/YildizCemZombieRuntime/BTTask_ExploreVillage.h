#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExploreVillage.generated.h"

// NodeMemory: InProgress görev boyunca durum tutmak için
struct FExploreTaskMemory
{
	FVector TargetLocation;   // Şu an gidilen nokta
	float   WaitStartTime;    // Bekleme başlangıç zamanı
	bool    bIsWaiting;       // Tüm evler gezildi, bekleme modunda mı

	FExploreTaskMemory()
		: TargetLocation(FVector::ZeroVector)
		, WaitStartTime(-1.0f)
		, bIsWaiting(false)
	{}
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

	UPROPERTY(EditAnywhere, Category = "Exploration")
	float WaitTimeAfterFullExploration = 60.0f;

private:
	bool ExploreCluster(UBehaviorTreeComponent& OwnerComp, int32 ClusterIndex,
						const FVector& CurrentLocation, FVector& OutTargetLocation);
};