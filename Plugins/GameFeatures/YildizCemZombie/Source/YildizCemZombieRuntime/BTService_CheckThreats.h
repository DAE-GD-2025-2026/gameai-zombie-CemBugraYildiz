#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CheckThreats.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTService_CheckThreats : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_CheckThreats();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float CriticalThreatDistance = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Threat")
	int32 MaxZombiesBeforeFlee = 3;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float LowHealthThreshold = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Threat")
	float LowAmmoThreshold = 5.0f;

private:
	bool ShouldFleeFromZombies(int32 ZombieCount, float NearestZombieDistance, bool bHasWeapon, float CurrentAmmo, float CurrentHealth) const;
};