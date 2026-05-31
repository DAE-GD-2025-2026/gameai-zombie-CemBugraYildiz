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

	const float CurrentHealth = BlackboardComp->GetValueAsFloat(HealthKey.SelectedKeyName);
	
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
	
	if (bResult)
	{
		UE_LOG(LogTemp, Warning, TEXT("❤️ [HEALTH CHECK] Health %.1f %s %.1f = TRUE"), 
			CurrentHealth, 
			ComparisonType == EHealthComparison::Less ? TEXT("<") : TEXT(">"),
			HealthThreshold);
	}
	
	return bResult;
}