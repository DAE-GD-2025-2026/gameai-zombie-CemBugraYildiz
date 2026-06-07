#include "BTTask_MoveToRandomLocation.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"
#include "WanderSteering.h"
#include "ExplorationMemory.h" 

UBTTask_MoveToRandomLocation::UBTTask_MoveToRandomLocation()
{
    NodeName = TEXT("Move To Random Location (Smart Explore)");
    SearchRadius = 2000.0f;
    Cooldown = 10.0f;
    bUseWanderSteering = true;
    WanderDuration = 10.0f;
    bUseClusterExploration = true;         
    bAvoidRecentlyVisited = true;           
    VisitedLocationAvoidanceRadius = 1500.0f; 
    
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToRandomLocation::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    float LastMoveTime = Blackboard->GetValueAsFloat(LastMoveTimeKey.SelectedKeyName);
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastMoveTime < Cooldown) 
    {
        return EBTNodeResult::Failed;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();

    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();

    if (ExpMemory)
    {
        ExpMemory->RecordVisitedLocation(PawnLocation);
    }

    if (bUseWanderSteering)
    {
        ExploreUsingWander(OwnerComp);
        WanderStartTime = CurrentTime;
        Blackboard->SetValueAsFloat(LastMoveTimeKey.SelectedKeyName, CurrentTime);
        
        return EBTNodeResult::InProgress;
    }

    return EBTNodeResult::Failed;
}

void UBTTask_MoveToRandomLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    if (!bUseWanderSteering)
    {
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - WanderStartTime;

    if (ElapsedTime >= WanderDuration)
    {
        AAIController* AIController = OwnerComp.GetAIOwner();
        if (AIController && AIController->GetPawn())
        {
            APawn* Pawn = AIController->GetPawn();
            
            UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
            if (ExpMemory)
            {
                ExpMemory->RecordVisitedLocation(Pawn->GetActorLocation());
            }
            
            USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
            if (SteeringComp)
            {
                SteeringComp->ClearAllBehaviors();
                SteeringComp->bAutoApplySteering = false;
            }
        }
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

void UBTTask_MoveToRandomLocation::ExploreUsingWander(UBehaviorTreeComponent& OwnerComp)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return;
    }

    APawn* Pawn = AIController->GetPawn();
    AIController->StopMovement();

    USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
    if (!SteeringComp)
    {
        SteeringComp = NewObject<USteeringComponent>(Pawn);
        SteeringComp->RegisterComponent();
        Pawn->AddInstanceComponent(SteeringComp);
    }

    SteeringComp->ClearAllBehaviors();

    UWanderSteering* WanderBehavior = NewObject<UWanderSteering>();
    WanderBehavior->WanderRadius = 200.0f;
    WanderBehavior->WanderDistance = 400.0f;
    WanderBehavior->WanderJitter = 100.0f;
    WanderBehavior->MaxSpeed = 400.0f;
    WanderBehavior->Weight = 1.0f;

    SteeringComp->AddSteeringBehavior(WanderBehavior);
    SteeringComp->bAutoApplySteering = true;
    SteeringComp->MaxSpeed = 400.0f;
    SteeringComp->bDrawDebug = false;
}

EBTNodeResult::Type UBTTask_MoveToRandomLocation::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (bUseWanderSteering)
    {
        AAIController* AIController = OwnerComp.GetAIOwner();
        if (AIController && AIController->GetPawn())
        {
            APawn* Pawn = AIController->GetPawn();
            USteeringComponent* SC = Pawn->FindComponentByClass<USteeringComponent>();
            if (SC)
            {
                SC->ClearAllBehaviors();
                SC->bAutoApplySteering = false;
            }
        }
    }
    return Super::AbortTask(OwnerComp, NodeMemory);
}