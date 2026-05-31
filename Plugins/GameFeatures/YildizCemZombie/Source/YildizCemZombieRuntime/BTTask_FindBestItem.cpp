#include "BTTask_FindBestItem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindBestItem::UBTTask_FindBestItem()
{
    NodeName = TEXT("Find Best Item");
}

EBTNodeResult::Type UBTTask_FindBestItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    AActor* BestItem = Cast<AActor>(BlackboardComp->GetValueAsObject(FName("BestItem")));

    if (BestItem)
    {
        if (BestItemKey.SelectedKeyName != NAME_None)
        {
            BlackboardComp->SetValueAsObject(BestItemKey.SelectedKeyName, BestItem);
        }
        
        UE_LOG(LogTemp, Log, TEXT("✅ [FIND ITEM] Best item found: %s"), *BestItem->GetName());
        return EBTNodeResult::Succeeded;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ℹ️ [FIND ITEM] No suitable items nearby"));
        return EBTNodeResult::Failed;
    }
}