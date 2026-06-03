#include "BTDecorator_HasWeapon.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasWeapon::UBTDecorator_HasWeapon()
{
	NodeName = TEXT("Has Weapon & Ammo");
	MinAmmoRequired = 3;
}

bool UBTDecorator_HasWeapon::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	const bool bHasWeapon = BlackboardComp->GetValueAsBool(FName("HasWeapon"));
    
	const int32 CurrentAmmo = BlackboardComp->GetValueAsInt(FName("WeaponAmmo"));
    
	const bool bCanFight = bHasWeapon && (CurrentAmmo >= MinAmmoRequired);
    
    
	return bCanFight;
}