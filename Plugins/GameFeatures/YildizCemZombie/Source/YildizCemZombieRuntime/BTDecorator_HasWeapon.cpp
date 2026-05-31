#include "BTDecorator_HasWeapon.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasWeapon::UBTDecorator_HasWeapon()
{
	NodeName = TEXT("Has Weapon & Ammo");
	MinAmmoRequired = 3.0f;
}

bool UBTDecorator_HasWeapon::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	const bool bHasWeapon = BlackboardComp->GetValueAsBool(FName("HasWeapon"));
    
	const float CurrentAmmo = BlackboardComp->GetValueAsFloat(FName("CurrentAmmo"));
    
	const bool bCanFight = bHasWeapon && (CurrentAmmo >= MinAmmoRequired);
    
	UE_LOG(LogTemp, Log, TEXT("🔫 HasWeapon: %s, Ammo: %.0f, CanFight: %s"),
		bHasWeapon ? TEXT("Yes") : TEXT("No"),
		CurrentAmmo,
		bCanFight ? TEXT("Yes") : TEXT("No")
	);
    
	return bCanFight;
}