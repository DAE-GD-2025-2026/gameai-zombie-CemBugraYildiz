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
        UE_LOG(LogTemp, Warning, TEXT("❌ [VILLAGE] No ExplorationMemory component"));
        return EBTNodeResult::Failed;
    }

    const FVector CurrentLocation = Pawn->GetActorLocation();

    int32 CurrentClusterIndex = -1;
    if (ExpMemory->IsInsideAnyCluster(CurrentLocation, CurrentClusterIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("🏘️ [VILLAGE] Inside cluster %d, exploring..."), CurrentClusterIndex);
        
        if (ExploreCluster(OwnerComp, CurrentClusterIndex, CurrentLocation))
        {
            return EBTNodeResult::Succeeded;
        }
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
                UE_LOG(LogTemp, Warning, TEXT("🏘️ [VILLAGE] Moving to village with %d houses"), HouseCount);
                return EBTNodeResult::Succeeded;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ℹ️ [VILLAGE] No unexplored villages found"));
    return EBTNodeResult::Failed;
}

bool UBTTask_ExploreVillage::ExploreCluster(UBehaviorTreeComponent& OwnerComp, int32 ClusterIndex, const FVector& CurrentLocation)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return false;
    }

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    
    if (!ExpMemory)
    {
        return false;
    }

    FVector ClusterCenter;
    float ClusterRadius;
    int32 HouseCount;
    bool bExplored;
    
    if (!ExpMemory->GetClusterInfo(ClusterIndex, ClusterCenter, ClusterRadius, HouseCount, bExplored))
    {
        return false;
    }

    const int32 PointCount = 8;
    float MinDistance = FLT_MAX;
    FVector BestPoint = ClusterCenter;

    for (int32 i = 0; i < PointCount; i++)
    {
        const float Angle = (360.0f / PointCount) * i;
        const float Radius = ClusterRadius * 0.7f;
        
        FVector TestPoint = ClusterCenter + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.0f
        );

        if (!ExpMemory->IsLocationVisitedRecently(TestPoint, HouseVisitDistance, 300.0f))
        {
            const float Distance = FVector::Dist(CurrentLocation, TestPoint);
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                BestPoint = TestPoint;
            }
        }
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return false;
    }

    FNavLocation NavLocation;
    if (NavSys->ProjectPointToNavigation(BestPoint, NavLocation, FVector(1000.0f)))
    {
        EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
            NavLocation.Location,
            100.0f
        );
        
        if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
        {
            UE_LOG(LogTemp, Warning, TEXT("🏘️ [VILLAGE] Exploring point in village"));
            
            if (bMarkAsExploredWhenDone)
            {
                bool bAllVisited = true;
                for (int32 i = 0; i < PointCount; i++)
                {
                    const float Angle = (360.0f / PointCount) * i;
                    const float Radius = ClusterRadius * 0.7f;
                    
                    FVector TestPoint = ClusterCenter + FVector(
                        FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
                        FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
                        0.0f
                    );

                    if (!ExpMemory->IsLocationVisitedRecently(TestPoint, HouseVisitDistance, 600.0f))
                    {
                        bAllVisited = false;
                        break;
                    }
                }

                if (bAllVisited)
                {
                    ExpMemory->MarkClusterAsExplored(ClusterIndex);
                    UE_LOG(LogTemp, Warning, TEXT("✅ [VILLAGE] Village fully explored!"));
                }
            }
            
            return true;
        }
    }

    return false;
}