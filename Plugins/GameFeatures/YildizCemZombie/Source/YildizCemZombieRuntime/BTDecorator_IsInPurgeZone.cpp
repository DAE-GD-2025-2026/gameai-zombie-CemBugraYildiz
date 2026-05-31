#include "BTDecorator_IsInPurgeZone.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_IsInPurgeZone::UBTDecorator_IsInPurgeZone()
{
	NodeName = TEXT("Is In Purge Zone");
	
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
}

bool UBTDecorator_IsInPurgeZone::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return false;
	}

	AActor* PurgeZone = Cast<AActor>(BlackboardComp->GetValueAsObject(PurgeZoneKey.SelectedKeyName));
	
	const bool bInPurgeZone = (PurgeZone != nullptr);
	
	if (bInPurgeZone)
	{
		UE_LOG(LogTemp, Error, TEXT("☠️ [DECORATOR] PURGE ZONE ACTIVE - HIGHEST PRIORITY!"));
	}
	
	return bInPurgeZone;
}