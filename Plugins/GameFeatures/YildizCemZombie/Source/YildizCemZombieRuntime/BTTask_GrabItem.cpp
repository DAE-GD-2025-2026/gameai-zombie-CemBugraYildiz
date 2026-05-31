#include "BTTask_GrabItem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_GrabItem::UBTTask_GrabItem()
{
    NodeName = TEXT("Grab Item");
    GrabDistance = 300.0f;
    
    bNotifyTick = false; 
}

EBTNodeResult::Type UBTTask_GrabItem::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ [GRAB] No AI Controller or Pawn"));
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
        UE_LOG(LogTemp, Log, TEXT("ℹ️ [GRAB] No best item available"));
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetItem->GetActorLocation());
    
    FString ClassName = TargetItem->GetClass()->GetName();
    bool bIsGarbage = ClassName.Contains(TEXT("Garbage"));

    if (Distance <= GrabDistance)
    {
        if (bIsGarbage)
        {
            TargetItem->Destroy();
            BlackboardComp->ClearValue(FName("BestItem"));
            
            UE_LOG(LogTemp, Warning, TEXT("🗑️ [GRAB] Garbage removed to clear spawn point: %s"), 
                *TargetItem->GetName());
            
            return EBTNodeResult::Succeeded; 
        }

        UActorComponent* InventoryComp = Pawn->GetComponentByClass(UActorComponent::StaticClass());
        
        if (!InventoryComp)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ [GRAB] No component found on Pawn!"));
            BlackboardComp->ClearValue(FName("BestItem"));
            return EBTNodeResult::Failed; 
        }

        bool bInteractSucceeded = false;

        UFunction* InteractFunc = InventoryComp->FindFunction(FName("Interact"));
        if (InteractFunc)
        {
            struct FInteractParams
            {
                AActor* Item;
            };
            
            FInteractParams Params;
            Params.Item = TargetItem;
            
            InventoryComp->ProcessEvent(InteractFunc, &Params);
            bInteractSucceeded = true;
            
            UE_LOG(LogTemp, Warning, TEXT("✅ [GRAB] Item picked up via Interact: %s"), 
                *TargetItem->GetName());
        }
        else
        {
            UFunction* GrabFunc = InventoryComp->FindFunction(FName("GrabItem"));
            if (GrabFunc)
            {
                struct FGrabParams
                {
                    AActor* Item;
                };
                
                FGrabParams Params;
                Params.Item = TargetItem;
                
                InventoryComp->ProcessEvent(GrabFunc, &Params);
                bInteractSucceeded = true;
                
                UE_LOG(LogTemp, Warning, TEXT("✅ [GRAB] Item picked up via GrabItem: %s"), 
                    *TargetItem->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ [GRAB] No Interact/GrabItem function found!"));
            }
        }
        
        BlackboardComp->ClearValue(FName("BestItem"));
        
        if (bInteractSucceeded)
        {
            return EBTNodeResult::Succeeded;
        }
        else
        {
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
            UE_LOG(LogTemp, Log, TEXT("🚶 [GRAB] Moving to item: %s (Distance: %.1f)"), 
                *TargetItem->GetName(), Distance);
            
            return EBTNodeResult::InProgress; 
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ [GRAB] Failed to move to item"));
            return EBTNodeResult::Failed; 
        }
    }

}