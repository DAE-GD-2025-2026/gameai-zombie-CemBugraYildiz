#include "BTTask_FleeFromDanger.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"     
#include "FleeSteering.h"            

UBTTask_FleeFromDanger::UBTTask_FleeFromDanger()
{
    NodeName = TEXT("Flee From Danger (Zombies)");
    FleeDistance = 1500.0f;
    SearchRadius = 3000.0f;
    bPreferHousesWhenFleeing = true;
    bUseSteeringBehavior = true; 
    
    bNotifyTick = true;
    
    DangerActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FleeFromDanger, DangerActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FleeFromDanger::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    AActor* DangerActor = Cast<AActor>(BlackboardComp->GetValueAsObject(DangerActorKey.SelectedKeyName));
    
    if (!DangerActor)
    {
        UE_LOG(LogTemp, Log, TEXT("ℹ️ [FLEE] No danger actor found"));
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector DangerLocation = DangerActor->GetActorLocation();
    const float CurrentDistance = FVector::Dist(PawnLocation, DangerLocation);

    const float SafeDistance = FleeDistance;
    if (CurrentDistance > SafeDistance)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ [FLEE] Safe distance reached (%.0fm > %.0fm)"), 
            CurrentDistance, SafeDistance);
        
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false);
        
        return EBTNodeResult::Succeeded;
    }

    if (bUseSteeringBehavior)
    {
        FleeUsingSteering(OwnerComp, DangerActor);
        UE_LOG(LogTemp, Warning, TEXT("🏃 [FLEE] Using steering behavior to escape"));
        return EBTNodeResult::InProgress; 
    }

    FVector TargetLocation;
    bool bFoundTarget = false;

    if (bPreferHousesWhenFleeing)
    {
        AActor* SafeHouse = FindNearestSafeHouse(BlackboardComp, PawnLocation, DangerActor);
        
        if (SafeHouse)
        {
            float HouseDistanceFromZombie = FVector::Dist(SafeHouse->GetActorLocation(), DangerLocation);
            
            if (HouseDistanceFromZombie > CurrentDistance)
            {
                TargetLocation = SafeHouse->GetActorLocation();
                bFoundTarget = true;
                
                UE_LOG(LogTemp, Warning, TEXT("🏠 [FLEE] Fleeing to safe house: %s"), 
                    *SafeHouse->GetName());
            }
        }
    }

    if (!bFoundTarget)
    {
        const FVector FleeDirection = (PawnLocation - DangerLocation).GetSafeNormal();
        TargetLocation = PawnLocation + (FleeDirection * FleeDistance);
        bFoundTarget = true;
        
        UE_LOG(LogTemp, Warning, TEXT("🏃 [FLEE] Fleeing away from zombie"));
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return EBTNodeResult::Failed;
    }

    FNavLocation ResultLocation;
    const bool bSuccess = NavSys->ProjectPointToNavigation(
        TargetLocation,
        ResultLocation,
        FVector(SearchRadius, SearchRadius, SearchRadius)
    );

    if (bSuccess)
    {
        EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
            ResultLocation.Location,
            100.0f
        );
        
        if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
        {
            UE_LOG(LogTemp, Warning, TEXT("🏃 [FLEE] Escaping from %s (Distance: %.0fm)"), 
                *DangerActor->GetName(), CurrentDistance);
            
            return EBTNodeResult::InProgress;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("❌ [FLEE] Failed to find escape route"));
    return EBTNodeResult::Failed;
}

void UBTTask_FleeFromDanger::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    if (!bUseSteeringBehavior)
    {
        return; 
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();
    
    if (!BlackboardComp || !AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AActor* DangerActor = Cast<AActor>(BlackboardComp->GetValueAsObject(DangerActorKey.SelectedKeyName));
    APawn* Pawn = AIController->GetPawn();

    if (!DangerActor)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float Distance = FVector::Dist(Pawn->GetActorLocation(), DangerActor->GetActorLocation());
    
    if (Distance > FleeDistance)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ [FLEE STEERING] Safe distance reached!"));
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false);
        
        USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
        if (SteeringComp)
        {
            SteeringComp->ClearAllBehaviors();
        }
        
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UBTTask_FleeFromDanger::FleeUsingSteering(UBehaviorTreeComponent& OwnerComp, AActor* Threat)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return;
    }

    APawn* Pawn = AIController->GetPawn();

    USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
    if (!SteeringComp)
    {
        SteeringComp = NewObject<USteeringComponent>(Pawn);
        SteeringComp->RegisterComponent();
        Pawn->AddInstanceComponent(SteeringComp);
    }

    SteeringComp->ClearAllBehaviors();

    UFleeSteering* FleeBehavior = NewObject<UFleeSteering>();
    FleeBehavior->SetThreatActor(Threat);
    FleeBehavior->PanicDistance = FleeDistance;
    FleeBehavior->MaxFleeDistance = FleeDistance * 1.5f;
    FleeBehavior->MaxSpeed = 600.0f;
    FleeBehavior->Weight = 2.0f; 

    SteeringComp->AddSteeringBehavior(FleeBehavior);
    SteeringComp->bAutoApplySteering = true;
    SteeringComp->MaxSpeed = 600.0f;
}

AActor* UBTTask_FleeFromDanger::FindNearestSafeHouse(UBlackboardComponent* Blackboard, const FVector& PawnLocation, AActor* Threat) const
{
    if (!Blackboard || !Threat)
    {
        return nullptr;
    }

    AActor* SafeHouse = Cast<AActor>(Blackboard->GetValueAsObject(FName("SafeHouse")));
    
    if (SafeHouse)
    {
        float DistanceFromThreat = FVector::Dist(SafeHouse->GetActorLocation(), Threat->GetActorLocation());
        float DistanceToPawn = FVector::Dist(SafeHouse->GetActorLocation(), PawnLocation);
        
        if (DistanceToPawn < 3000.0f && DistanceFromThreat > FleeDistance)
        {
            return SafeHouse;
        }
    }

    return nullptr;
}