#include "BTDecorator_HealthCheck.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HealthCheck::UBTDecorator_HealthCheck()
{
	NodeName = TEXT("Health Check");
	HealthThreshold = 30.0f;
	ComparisonType = EHealthComparison::Less;
}

bool UBTDecorator_HealthCheck::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	float CurrentHealth = 0.0f;
    
	if (HealthKey.SelectedKeyName == FName("CurrentHealth"))
	{
		int32 HealthInt = BlackboardComp->GetValueAsInt(HealthKey.SelectedKeyName);
		CurrentHealth = (float)HealthInt;
	}
	else
	{
		CurrentHealth = BlackboardComp->GetValueAsFloat(HealthKey.SelectedKeyName);
	}
    
	bool bResult = false;
    
	switch (ComparisonType)
	{
	case EHealthComparison::Less:
		bResult = (CurrentHealth < HealthThreshold);
		break;
        
	case EHealthComparison::LessOrEqual:
		bResult = (CurrentHealth <= HealthThreshold);
		break;
        
	case EHealthComparison::Greater:
		bResult = (CurrentHealth > HealthThreshold);
		break;
        
	case EHealthComparison::GreaterOrEqual:
		bResult = (CurrentHealth >= HealthThreshold);
		break;
	}
	return bResult;
}