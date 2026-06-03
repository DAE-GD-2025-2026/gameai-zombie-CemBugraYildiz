#include "BTTask_GrabItem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_GrabItem::UBTTask_GrabItem()
{
    NodeName = TEXT("Grab Item");
    GrabDistance = 300.0f;
    
    bNotifyTick = true; 
}

EBTNodeResult::Type UBTTask_GrabItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    AActor* TargetItem = Cast<AActor>(BlackboardComp->GetValueAsObject(FName("BestItem")));
    
    if (!TargetItem)
    {
        return EBTNodeResult::Failed;
    }

    FGrabItemTaskMemory* Memory = CastInstanceNodeMemory<FGrabItemTaskMemory>(NodeMemory);
    Memory->CachedTarget = TargetItem;
    Memory->TaskStartTime = GetWorld()->GetTimeSeconds();
    APawn* Pawn = AIController->GetPawn();
    const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetItem->GetActorLocation());
    

    if (Distance <= GrabDistance)
    {
        UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
        
        if (!InventoryComp)
        {
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Failed;
        }

        ABaseItem* Item = Cast<ABaseItem>(TargetItem);
        
        if (!Item)
        {
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Failed;
        }

        if (Item->GetItemType() == EItemType::Garbage)
        {
            Item->Destroy();
            BlackboardComp->ClearValue(FName("BestItem"));
            
            return EBTNodeResult::Succeeded;
        }

        int32 EmptySlot = FindEmptySlot(InventoryComp);
        
        if (EmptySlot == -1)
        {
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Failed;
        }

        bool bSuccess = InventoryComp->GrabItem(EmptySlot, Item);
        
        if (bSuccess)
        {
            FString ItemTypeName;
            switch (Item->GetItemType())
            {
                case EItemType::Medkit: ItemTypeName = TEXT("Medkit"); break;
                case EItemType::Food: ItemTypeName = TEXT("Food"); break;
                case EItemType::Pistol: ItemTypeName = TEXT("Pistol"); break;
                case EItemType::Shotgun: ItemTypeName = TEXT("Shotgun"); break;
                default: ItemTypeName = TEXT("Unknown"); break;
            }
            
            
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Succeeded;
        }
        else
        {
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Failed;
        }
    }
    else 
    {
        FAIMoveRequest MoveRequest(TargetItem);
        MoveRequest.SetAcceptanceRadius(GrabDistance - 50.0f);
        
        FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);
        
        if (MoveResult.Code == EPathFollowingRequestResult::RequestSuccessful)
        {
            
            return EBTNodeResult::InProgress;
        }
        else
        {
            return EBTNodeResult::Failed;
        }
    }
}

void UBTTask_GrabItem::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    /*if (!BlackboardComp)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }*/

    FGrabItemTaskMemory* Memory = CastInstanceNodeMemory<FGrabItemTaskMemory>(NodeMemory);
    AActor* TargetItem = Memory->CachedTarget.Get();
    APawn* Pawn = AIController->GetPawn();

    /*if (!TargetItem || !TargetItem->IsValidLowLevel())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }*/

    const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetItem->GetActorLocation());
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - Memory->TaskStartTime;

    if (ElapsedTime > 8.0f)
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (Distance > GrabDistance)
    {
        float PawnSpeed = Pawn->GetVelocity().Size();
        if (PawnSpeed < 10.0f && ElapsedTime > 2.0f)
        {
            BlackboardComp->ClearValue(FName("BestItem"));
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }
        return;
    }

    UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
    if (!InventoryComp)
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    ABaseItem* Item = Cast<ABaseItem>(TargetItem);
    if (!Item)
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (Item->GetItemType() == EItemType::Garbage)
    {
        Item->Destroy();
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    int32 EmptySlot = FindEmptySlot(InventoryComp);
    if (EmptySlot == -1)
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (InventoryComp->GrabItem(EmptySlot, Item))
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
    else
    {
        BlackboardComp->ClearValue(FName("BestItem"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    }
}

int32 UBTTask_GrabItem::FindEmptySlot(UInventoryComponent* InventoryComp) const
{
    if (!InventoryComp)
    {
        return -1;
    }

    const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
    
    for (int32 i = 0; i < Items.Num(); i++)
    {
        if (Items[i] == nullptr)
        {
            return i;
        }
    }
    
    return -1; 
}