#include "BTDecorator_CheckAmmo.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CheckAmmo::UBTDecorator_CheckAmmo()
{
	NodeName = TEXT("Check Ammo");
	MinAmmo = 5;
}

bool UBTDecorator_CheckAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	const int32 CurrentAmmo = BlackboardComp->GetValueAsInt(AmmoKey.SelectedKeyName);
    
	const bool bHasEnoughAmmo = (CurrentAmmo >= MinAmmo);
    
	return bHasEnoughAmmo;
}