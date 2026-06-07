#include "BTTask_MoveToSafeHouse.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "Village/House/House.h"

UBTTask_MoveToSafeHouse::UBTTask_MoveToSafeHouse()
{
    NodeName = TEXT("Move To Safe House");
    EnterDistance = 500.0f;
    
    SafeHouseKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToSafeHouse, SafeHouseKey), AActor::StaticClass());
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToSafeHouse::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    AActor* SafeHouse = Cast<AActor>(BlackboardComp->GetValueAsObject(SafeHouseKey.SelectedKeyName));
    
    if (!SafeHouse)
    {
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector HouseLocation = SafeHouse->GetActorLocation();
    const float Distance = FVector::Dist(PawnLocation, HouseLocation);

    if (IsInsideHouse(Pawn, SafeHouse))
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [SAFE HOUSE] Inside house, safe from zombies!"));
        
        BlackboardComp->SetValueAsBool(FName("IsInsideHouse"), true);
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false); 
        
        return EBTNodeResult::Succeeded;
    }

    if (Distance > EnterDistance)
    {
        FAIMoveRequest MoveRequest(HouseLocation);
        MoveRequest.SetAcceptanceRadius(EnterDistance * 0.5f);
        
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
    else
    {
        BlackboardComp->SetValueAsBool(FName("IsInsideHouse"), true);
        return EBTNodeResult::Succeeded;
    }
}

void UBTTask_MoveToSafeHouse::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn()) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AActor* SafeHouse = BB ? Cast<AActor>(BB->GetValueAsObject(SafeHouseKey.SelectedKeyName)) : nullptr;
    APawn* Pawn = AIController->GetPawn();

    if (!SafeHouse) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    if (IsInsideHouse(Pawn, SafeHouse))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

bool UBTTask_MoveToSafeHouse::IsInsideHouse(APawn* Pawn, AActor* House) const
{
    if (!Pawn || !House)
    {
        return false;
    }

    AHouse* HouseActor = Cast<AHouse>(House);
    if (HouseActor)
    {
        FHouseBounds Bounds = HouseActor->GetBounds();
        FVector PawnLoc = Pawn->GetActorLocation();
        
        bool bInsideX = FMath::Abs(PawnLoc.X - Bounds.Origin.X) < Bounds.Extent.X;
        bool bInsideY = FMath::Abs(PawnLoc.Y - Bounds.Origin.Y) < Bounds.Extent.Y;
        bool bInsideZ = FMath::Abs(PawnLoc.Z - Bounds.Origin.Z) < Bounds.Extent.Z;
        
        if (bInsideX && bInsideY && bInsideZ)
        {
            return true;
        }
    }
    
    const float Distance = FVector::Dist(Pawn->GetActorLocation(), House->GetActorLocation());
    
    if (Distance < EnterDistance * 0.3f) 
    {
        return true;
    }

    return false;
}