#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackTarget.generated.h"

class UInventoryComponent;
class ABaseItem;
class AWeapon;

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_AttackTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MinAmmoToFight = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering")
	bool bUseSeekSteering = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireInterval = 0.5f;

private:
	void FireWeapon(APawn* Pawn, AActor* Target) const;
	bool HasEnoughAmmo(APawn* Pawn) const;
	bool HasLineOfSight(APawn* Pawn, AActor* Target) const;
	void ApproachUsingSeek(UBehaviorTreeComponent& OwnerComp, AActor* Target);
	
	float LastFireTime = 0.0f; 
};