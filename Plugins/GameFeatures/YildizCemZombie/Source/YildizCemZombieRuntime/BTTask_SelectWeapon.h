#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectWeapon.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTTask_SelectWeapon : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SelectWeapon();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FBlackboardKeySelector TargetZombieKey;

private:
	FString GetBestWeaponForTarget(AActor* Zombie, int32 NearbyZombieCount) const;
	void EquipWeapon(APawn* Pawn, const FString& WeaponType) const;
};