#include "BTTask_UseConsumable.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_UseConsumable::UBTTask_UseConsumable()
{
	NodeName = TEXT("Use Consumable");
}

EBTNodeResult::Type UBTTask_UseConsumable::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn()) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!InventoryComp) return EBTNodeResult::Failed;

	const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i]->GetItemType() == ItemTypeToUse)
		{
			if (InventoryComp->UseItem(i))
			{
				InventoryComp->RemoveItem(i);
				return EBTNodeResult::Succeeded;
			}
		}
	}
	return EBTNodeResult::Failed;
}