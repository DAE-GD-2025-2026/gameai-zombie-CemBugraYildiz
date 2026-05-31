#include "BTDecorator_CheckAmmo.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CheckAmmo::UBTDecorator_CheckAmmo()
{
	NodeName = TEXT("Check Ammo");
	MinAmmo = 5.0f;
}

bool UBTDecorator_CheckAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	const float CurrentAmmo = BlackboardComp->GetValueAsFloat(AmmoKey.SelectedKeyName);
	
	const bool bHasEnoughAmmo = (CurrentAmmo >= MinAmmo);
	
	if (!bHasEnoughAmmo)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔫 [AMMO CHECK] Low ammo: %.0f < %.0f"), CurrentAmmo, MinAmmo);
	}
	
	return bHasEnoughAmmo;
}