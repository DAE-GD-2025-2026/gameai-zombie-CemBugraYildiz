#include "BTTask_ExploreVillage.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "ExplorationMemory.h"

UBTTask_ExploreVillage::UBTTask_ExploreVillage()
{
    NodeName = TEXT("Explore Village");
    ExplorationRadius = 1500.0f;
    HouseVisitDistance = 500.0f;
    bMarkAsExploredWhenDone = true;
}

EBTNodeResult::Type UBTTask_ExploreVillage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    
    if (!ExpMemory)
    {
        return EBTNodeResult::Failed;
    }

    const FVector CurrentLocation = Pawn->GetActorLocation();

    int32 CurrentClusterIndex = -1;
    if (ExpMemory->IsInsideAnyCluster(CurrentLocation, CurrentClusterIndex))
    {
        ExploreCluster(OwnerComp, CurrentClusterIndex, CurrentLocation);
        return EBTNodeResult::Succeeded;
    }

    int32 NearestClusterIndex = ExpMemory->FindNearestUnexploredClusterIndex(CurrentLocation);
    
    if (NearestClusterIndex >= 0)
    {
        FVector ClusterCenter;
        float ClusterRadius;
        int32 HouseCount;
        bool bExplored;
        
        if (ExpMemory->GetClusterInfo(NearestClusterIndex, ClusterCenter, ClusterRadius, HouseCount, bExplored))
        {
            EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
                ClusterCenter,
                HouseVisitDistance
            );
            
            if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
            {
                return EBTNodeResult::Succeeded;
            }
        }
    }
    return EBTNodeResult::Failed;
}

bool UBTTask_ExploreVillage::ExploreCluster(UBehaviorTreeComponent& OwnerComp,
    int32 ClusterIndex, const FVector& CurrentLocation)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastExploreTime < MinExploreInterval)
        return false; 

    LastExploreTime = CurrentTime;

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn()) return false;

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    if (!ExpMemory) return false;

    FHouseCluster* Cluster = ExpMemory->GetClusterByIndex(ClusterIndex);
    if (!Cluster || Cluster->Houses.Num() == 0) return false;

    FVector BestPoint = FVector::ZeroVector;
    float MinDistance = FLT_MAX;

    for (AActor* House : Cluster->Houses)
    {
        if (!House) continue;
        FVector HouseLoc = House->GetActorLocation();

        if (!ExpMemory->IsLocationVisitedRecently(HouseLoc, 300.0f, 300.0f))
        {
            float Distance = FVector::Dist2D(CurrentLocation, HouseLoc);
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                BestPoint   = HouseLoc;
            }
        }
    }

    if (BestPoint.IsZero()) return false; 

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return false;

    FNavLocation NavLocation;
    if (NavSys->ProjectPointToNavigation(BestPoint, NavLocation, FVector(500.0f)))
    {
        EPathFollowingRequestResult::Type MoveResult =
            AIController->MoveToLocation(NavLocation.Location, 150.0f);

        if (MoveResult == EPathFollowingRequestResult::RequestSuccessful ||
            MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
        {
            ExpMemory->RecordVisitedLocation(BestPoint);

            if (bMarkAsExploredWhenDone)
            {
                bool bAllVisited = true;
                for (AActor* House : Cluster->Houses)
                {
                    if (!House) continue;
                    if (!ExpMemory->IsLocationVisitedRecently(
                            House->GetActorLocation(), 300.0f, 300.0f))
                    {
                        bAllVisited = false;
                        break;
                    }
                }
                if (bAllVisited) ExpMemory->MarkClusterAsExplored(ClusterIndex);
            }
            return true;
        }
    }
    return false;
}