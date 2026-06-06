#include "BTTask_ExploreVillage.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ExplorationMemory.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_ExploreVillage::UBTTask_ExploreVillage()
{
    NodeName = TEXT("Explore Village");
    ExplorationRadius = 1500.0f;
    HouseVisitDistance = 150.0f;
    bMarkAsExploredWhenDone = true;
    bNotifyTick = true; 
}

bool UBTTask_ExploreVillage::ExploreCluster(UBehaviorTreeComponent& OwnerComp,
    int32 ClusterIndex, const FVector& CurrentLocation, FVector& OutTargetLocation, AActor*& OutTargetHouse)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn()) return false;

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    if (!ExpMemory) return false;

    FHouseCluster* Cluster = ExpMemory->GetClusterByIndex(ClusterIndex);
    if (!Cluster || Cluster->Houses.Num() == 0) return false;

    float MinDistance = FLT_MAX;
    OutTargetLocation = FVector::ZeroVector;
    OutTargetHouse = nullptr;

    for (AActor* House : Cluster->Houses)
    {
        if (!House) continue;
        FVector HouseLoc = House->GetActorLocation();

        if (ExpMemory->IsHouseVisitedRecently(House, 60.0f))
            continue;

        float Distance = FVector::Dist2D(CurrentLocation, HouseLoc);
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            OutTargetLocation = HouseLoc;
            OutTargetHouse = House; 
        }
    }
    if (!OutTargetLocation.IsZero()) return true;
    UInventoryComponent* InvComp = Pawn->FindComponentByClass<UInventoryComponent>();
    bool bHasSpace = false;
    if (InvComp)
        for (ABaseItem* Slot : InvComp->GetInventory())
            if (!Slot) { bHasSpace = true; break; }

    if (bHasSpace && Cluster)
    {
        float MinDist = FLT_MAX;
        for (AActor* KnownItem : ExpMemory->GetKnownWorldItems())
        {
            if (!KnownItem || !IsValid(KnownItem)) continue;
            ABaseItem* BI = Cast<ABaseItem>(KnownItem);
            if (!BI || BI->GetItemType() == EItemType::Garbage) continue;
            float DistToCenter = FVector::Dist2D(KnownItem->GetActorLocation(), Cluster->CenterLocation);
            if (DistToCenter <= Cluster->Radius + 600.0f)
            {
                float DistToMe = FVector::Dist2D(CurrentLocation, KnownItem->GetActorLocation());
                if (DistToMe < MinDist)
                {
                    MinDist = DistToMe;
                    OutTargetLocation = KnownItem->GetActorLocation();
                    OutTargetHouse = nullptr;
                }
            }
        }
        if (!OutTargetLocation.IsZero()) return true;
    }
    return false;
}
EBTNodeResult::Type UBTTask_ExploreVillage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn()) return EBTNodeResult::Failed;

    APawn* Pawn = AIController->GetPawn();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    if (!ExpMemory) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    FExploreTaskMemory* Memory = CastInstanceNodeMemory<FExploreTaskMemory>(NodeMemory);

    *Memory = FExploreTaskMemory();

    if (BB && BB->GetValueAsObject(FName("BestItem")) != nullptr)
        return EBTNodeResult::Succeeded;

    const FVector CurrentLocation = Pawn->GetActorLocation();
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    FVector InitialTarget;
    bool bFoundTarget = false;

    int32 ClusterIdx;
    if (ExpMemory->IsInsideAnyCluster(CurrentLocation, ClusterIdx))
    {
        AActor* TargetHouse = nullptr;
        bFoundTarget = ExploreCluster(OwnerComp, ClusterIdx, CurrentLocation, InitialTarget, TargetHouse);
        if (!bFoundTarget)
            ExpMemory->MarkClusterAsExplored(ClusterIdx);
        else
            Memory->TargetHouseActor = TargetHouse;
    }

    if (!bFoundTarget)
    {
        int32 NextCluster = ExpMemory->FindNearestUnexploredClusterIndex(CurrentLocation);
        if (NextCluster >= 0)
        {
            FVector Center; float R; int32 HC; bool bEx;
            ExpMemory->GetClusterInfo(NextCluster, Center, R, HC, bEx);
            InitialTarget = Center;
            bFoundTarget = true;
        }
    }

    if (bFoundTarget && NavSys)
    {
        FNavLocation NavLoc;
        if (NavSys->ProjectPointToNavigation(InitialTarget, NavLoc, FVector(500.0f)))
        {
            AIController->MoveToLocation(NavLoc.Location, HouseVisitDistance);
            Memory->TargetLocation = NavLoc.Location;
        }
    }
    else
    {
        FNavLocation RandomLoc;
        if (NavSys && NavSys->GetRandomPointInNavigableRadius(CurrentLocation, 2000.0f, RandomLoc))
        {
            AIController->MoveToLocation(RandomLoc.Location, 100.0f);
            Memory->TargetLocation = RandomLoc.Location;
            return EBTNodeResult::InProgress; 
        }
        return EBTNodeResult::Failed;  
    }

    return EBTNodeResult::InProgress; 
}

void UBTTask_ExploreVillage::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* Pawn = AIController->GetPawn();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    UExplorationMemory* ExpMemory = Pawn->FindComponentByClass<UExplorationMemory>();
    FExploreTaskMemory* Memory = CastInstanceNodeMemory<FExploreTaskMemory>(NodeMemory);
    const FVector PawnLoc = Pawn->GetActorLocation();
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (BB && BB->GetValueAsObject(FName("BestItem")) != nullptr)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    if (Memory->bJustArrived)
    {
        if (CurrentTime - Memory->ArrivalTime >= 1.0f)
        {
            Memory->bJustArrived = false;
            Memory->TargetLocation = FVector::ZeroVector;
        }
        else
        {
            return;
        }
    }
    else if (Memory->TargetLocation != FVector::ZeroVector)
    {
        if (!Memory->TargetHouseActor && ExpMemory && ExpMemory->GetClusterCount() > 0)
        {
            int32 NearestCluster = ExpMemory->FindNearestUnexploredClusterIndex(PawnLoc);
            if (NearestCluster >= 0)
            {
                AIController->StopMovement();
                Memory->TargetLocation = FVector::ZeroVector;
                Memory->NavStartTime = -1.0f;
                return;
            }
        }
        
        float DistToTarget = FVector::Dist2D(PawnLoc, Memory->TargetLocation);
        if (DistToTarget < 200.0f)
        {
            if (IsValid(Memory->TargetHouseActor))
            {
                ExpMemory->MarkHouseVisited(Memory->TargetHouseActor);
                Memory->TargetHouseActor = nullptr;
            }
            ExpMemory->RecordVisitedLocation(Memory->TargetLocation);
            AIController->StopMovement();
            Memory->bJustArrived = true;
            Memory->ArrivalTime = CurrentTime;
            Memory->NavStartTime = -1.0f;
        }
        else if (Memory->NavStartTime >= 0.0f && CurrentTime - Memory->NavStartTime > 8.0f)
        {
            Memory->TargetLocation = FVector::ZeroVector;
            Memory->NavStartTime = -1.0f;
            AIController->StopMovement();
        }
        return;
    }

    FVector NextTarget;
    bool bFoundNext = false;

    AActor* TargetHouse = nullptr;
    int32 ClusterIdx;
    if (ExpMemory->IsInsideAnyCluster(PawnLoc, ClusterIdx))
    {
        bFoundNext = ExploreCluster(OwnerComp, ClusterIdx, PawnLoc, NextTarget, TargetHouse);
        if (!bFoundNext && bMarkAsExploredWhenDone)
            ExpMemory->MarkClusterAsExplored(ClusterIdx);
    }

    if (!bFoundNext)
    {
        int32 NextCluster = ExpMemory->FindNearestUnexploredClusterIndex(PawnLoc);
        if (NextCluster >= 0)
        {
            FVector Center; float R; int32 HC; bool bEx;
            ExpMemory->GetClusterInfo(NextCluster, Center, R, HC, bEx);
            NextTarget = Center;
            bFoundNext = true;
        }
    }

    if (bFoundNext)
    {
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        FNavLocation NavLoc;
        if (NavSys && NavSys->ProjectPointToNavigation(NextTarget, NavLoc, FVector(500.0f)))
        {
            AIController->MoveToLocation(NavLoc.Location, HouseVisitDistance);
            Memory->TargetLocation = NavLoc.Location;
            Memory->TargetHouseActor = TargetHouse;
            Memory->NavStartTime = CurrentTime;
            return;
        }
    }
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation RandomLoc;
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(PawnLoc, 2000.0f, RandomLoc))
    {
        AIController->MoveToLocation(RandomLoc.Location, 100.0f);
        Memory->TargetLocation = RandomLoc.Location;
        Memory->TargetHouseActor = nullptr;
        Memory->NavStartTime = CurrentTime;
    }
    else
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    }
}