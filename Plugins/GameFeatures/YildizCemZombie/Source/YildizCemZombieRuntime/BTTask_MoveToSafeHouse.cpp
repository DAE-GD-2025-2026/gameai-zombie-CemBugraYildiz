#include "BTTask_MoveToSafeHouse.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToSafeHouse::UBTTask_MoveToSafeHouse()
{
    NodeName = TEXT("Move To Safe House");
    EnterDistance = 500.0f;
    
    SafeHouseKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToSafeHouse, SafeHouseKey), AActor::StaticClass());
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
        UE_LOG(LogTemp, Warning, TEXT("❌ [SAFE HOUSE] No safe house available"));
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
            UE_LOG(LogTemp, Warning, TEXT("🏠 [SAFE HOUSE] Moving to house (Distance: %.1f)"), Distance);
            return EBTNodeResult::InProgress;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ [SAFE HOUSE] Cannot path to house"));
            return EBTNodeResult::Failed;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🚪 [SAFE HOUSE] Entering house..."));
        
        BlackboardComp->SetValueAsBool(FName("IsInsideHouse"), true);
        
        return EBTNodeResult::Succeeded;
    }
}

bool UBTTask_MoveToSafeHouse::IsInsideHouse(APawn* Pawn, AActor* House) const
{
    if (!Pawn || !House)
    {
        return false;
    }

    const float Distance = FVector::Dist(Pawn->GetActorLocation(), House->GetActorLocation());
    
    if (Distance < EnterDistance * 0.5f)
    {
        return true;
    }

    
    return false;
}