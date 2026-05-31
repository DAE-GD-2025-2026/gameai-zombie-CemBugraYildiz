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
        UE_LOG(LogTemp, Log, TEXT("⏳ [EXPLORE] Cooldown active (%.1fs remaining)"), 
            Cooldown - (CurrentTime - LastMoveTime));
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
        
        UE_LOG(LogTemp, Warning, TEXT("🎲 [EXPLORE] Starting wander behavior for %.1fs"), WanderDuration);
        return EBTNodeResult::InProgress;
    }

    FVector TargetLocation = FVector::ZeroVector;
    bool bFoundTarget = false;

    if (bUseClusterExploration && ExpMemory)
    {
        TargetLocation = FindClusterBasedExploration(OwnerComp, PawnLocation);
        if (!TargetLocation.IsZero())
        {
            bFoundTarget = true;
            UE_LOG(LogTemp, Warning, TEXT("🏘️ [EXPLORE] Cluster-based target found"));
        }
    }

    if (!bFoundTarget && bAvoidRecentlyVisited && ExpMemory)
    {
        TargetLocation = FindLeastVisitedDirection(OwnerComp, PawnLocation);
        if (!TargetLocation.IsZero())
        {
            bFoundTarget = true;
            UE_LOG(LogTemp, Warning, TEXT("🧭 [EXPLORE] Least visited direction chosen"));
        }
    }

    if (!bFoundTarget)
    {
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (!NavSys)
        {
            return EBTNodeResult::Failed;
        }

        FNavLocation RandomLocation;
        FVector SearchOrigin = PawnLocation;
        
        AActor* NearestHouse = Cast<AActor>(Blackboard->GetValueAsObject(FName("SafeHouse")));
        if (NearestHouse)
        {
            FVector ToHouse = (NearestHouse->GetActorLocation() - PawnLocation).GetSafeNormal();
            SearchOrigin = PawnLocation + (ToHouse * SearchRadius * 0.5f);
        }

        if (NavSys->GetRandomPointInNavigableRadius(SearchOrigin, SearchRadius, RandomLocation))
        {
            if (bAvoidRecentlyVisited && ExpMemory && 
                ExpMemory->IsLocationVisitedRecently(RandomLocation.Location, VisitedLocationAvoidanceRadius, 120.0f))
            {
                for (int32 i = 0; i < 3; i++)
                {
                    if (NavSys->GetRandomPointInNavigableRadius(SearchOrigin, SearchRadius, RandomLocation))
                    {
                        if (!ExpMemory->IsLocationVisitedRecently(RandomLocation.Location, VisitedLocationAvoidanceRadius, 120.0f))
                        {
                            break;
                        }
                    }
                }
            }

            TargetLocation = RandomLocation.Location;
            bFoundTarget = true;
        }
    }

    if (!bFoundTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ [EXPLORE] Failed to find exploration target"));
        return EBTNodeResult::Failed;
    }

    Blackboard->SetValueAsFloat(LastMoveTimeKey.SelectedKeyName, CurrentTime);
    
    if (TargetLocationKey.SelectedKeyName != NAME_None)
    {
        Blackboard->SetValueAsVector(TargetLocationKey.SelectedKeyName, TargetLocation);
    }

    EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
        TargetLocation, 
        100.0f
    );
    
    if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("🗺️ [EXPLORE] Moving to: %s"), *TargetLocation.ToString());
        return EBTNodeResult::Succeeded;
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

        UE_LOG(LogTemp, Log, TEXT("✅ [EXPLORE WANDER] Wander duration completed"));
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

FVector UBTTask_MoveToRandomLocation::FindClusterBasedExploration(UBehaviorTreeComponent& OwnerComp, const FVector& CurrentLocation)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return FVector::ZeroVector;
    }

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    
    if (!ExpMemory)
    {
        return FVector::ZeroVector;
    }

    int32 ClusterIndex = ExpMemory->FindNearestUnexploredClusterIndex(CurrentLocation);
    
    if (ClusterIndex >= 0)
    {
        FVector ClusterCenter;
        float ClusterRadius;
        int32 HouseCount;
        bool bExplored;
        
        if (ExpMemory->GetClusterInfo(ClusterIndex, ClusterCenter, ClusterRadius, HouseCount, bExplored))
        {
            UE_LOG(LogTemp, Warning, TEXT("🏘️ [CLUSTER EXPLORE] Targeting village with %d houses at %s"), 
                HouseCount, *ClusterCenter.ToString());
            
            return ClusterCenter;
        }
    }

    return FVector::ZeroVector;
}

FVector UBTTask_MoveToRandomLocation::FindLeastVisitedDirection(UBehaviorTreeComponent& OwnerComp, const FVector& CurrentLocation)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return FVector::ZeroVector;
    }

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    
    if (!ExpMemory)
    {
        return FVector::ZeroVector;
    }

    FVector BestDirection = ExpMemory->GetLeastVisitedAreaDirection(CurrentLocation, SearchRadius);
    FVector TargetLocation = CurrentLocation + (BestDirection * SearchRadius);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return FVector::ZeroVector;
    }

    FNavLocation NavLocation;
    if (NavSys->ProjectPointToNavigation(TargetLocation, NavLocation, FVector(SearchRadius)))
    {
        if (IsLocationSuitableForExploration(ExpMemory, NavLocation.Location))
        {
            UE_LOG(LogTemp, Warning, TEXT("🧭 [LEAST VISITED] Direction: %s"), *BestDirection.ToString());
            return NavLocation.Location;
        }
    }

    return FVector::ZeroVector;
}

bool UBTTask_MoveToRandomLocation::IsLocationSuitableForExploration(UExplorationMemory* Memory, const FVector& Location)
{
    if (!Memory || !bAvoidRecentlyVisited)
    {
        return true;
    }

    if (Memory->IsLocationVisitedRecently(Location, VisitedLocationAvoidanceRadius, 120.0f))
    {
        return false;
    }

    return true;
}