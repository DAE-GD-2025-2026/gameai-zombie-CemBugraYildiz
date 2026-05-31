#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateSurvivorStats.generated.h"

UCLASS()
class YILDIZCEMZOMBIERUNTIME_API UBTService_UpdateSurvivorStats : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateSurvivorStats();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	float GetSurvivorHealth(APawn* Pawn) const;
	float GetSurvivorStamina(APawn* Pawn) const;
	bool HasWeapon(APawn* Pawn) const;
	float GetCurrentAmmo(APawn* Pawn) const;
	
	class UActorComponent* GetHealthComponent(APawn* Pawn) const;
	class UActorComponent* GetStaminaComponent(APawn* Pawn) const;
	class UActorComponent* GetInventoryComponent(APawn* Pawn) const;
};