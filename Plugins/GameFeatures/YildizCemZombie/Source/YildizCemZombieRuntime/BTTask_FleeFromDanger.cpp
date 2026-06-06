#include "BTTask_FleeFromDanger.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"     
#include "FleeSteering.h"    
#include "Village/House/House.h"
#include "EngineUtils.h"
#include "GameFramework/FloatingPawnMovement.h"

UBTTask_FleeFromDanger::UBTTask_FleeFromDanger()
{
    NodeName = TEXT("Flee From Danger (Zombies)");
    FleeDistance = 1500.0f;
    SearchRadius = 3000.0f;
    bPreferHousesWhenFleeing = true;
    bUseSteeringBehavior = false; 
    
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
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector DangerLocation = DangerActor->GetActorLocation();
    const float CurrentDistance = FVector::Dist(PawnLocation, DangerLocation);
    FFleeTaskMemory* Memory = CastInstanceNodeMemory<FFleeTaskMemory>(NodeMemory);
    Memory->TaskStartTime = GetWorld()->GetTimeSeconds();
    const float SafeDistance = FleeDistance;
    if (CurrentDistance > SafeDistance)
    {
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false);
        return EBTNodeResult::Succeeded;
    }

    AIController->StopMovement();
    UFloatingPawnMovement* MoveComp = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
    if (MoveComp) MoveComp->MaxSpeed = 600.0f;

    AActor* FleeHouse = FindBestFleeHouse(PawnLocation, DangerActor);
    if (FleeHouse)
    {
        AIController->MoveToActor(FleeHouse, 150.0f);
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
            }
        }
    }

    if (!bFoundTarget)
    {
        const FVector FleeDirection = (PawnLocation - DangerLocation).GetSafeNormal();
        TargetLocation = PawnLocation + (FleeDirection * FleeDistance);
        bFoundTarget = true;
        
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
            return EBTNodeResult::InProgress;
        }
    }

    return EBTNodeResult::Failed;
}

void UBTTask_FleeFromDanger::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    APawn* Pawn = AIController->GetPawn();
    FFleeTaskMemory* Memory = CastInstanceNodeMemory<FFleeTaskMemory>(NodeMemory);

    AActor* DangerActor = Cast<AActor>(BB->GetValueAsObject(DangerActorKey.SelectedKeyName));
    if (!DangerActor)
    {
        BB->SetValueAsBool(FName("IsInDanger"), false);
        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
        if (MC) MC->MaxSpeed = 400.0f;
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    const FVector PawnLoc = Pawn->GetActorLocation();
    const float ZombieDist = FVector::Dist(PawnLoc, DangerActor->GetActorLocation());

    if (ZombieDist > FleeDistance)
    {
        BB->SetValueAsBool(FName("IsInDanger"), false);
        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
        if (MC) MC->MaxSpeed = 400.0f;
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    if (ZombieDist < ZombieFollowDistance)
    {
        AActor* NewHouse = FindBestFleeHouse(PawnLoc, DangerActor);
        if (NewHouse)
        {
            AIController->MoveToActor(NewHouse, 150.0f);
        }
    }

    if (GetWorld()->GetTimeSeconds() - Memory->TaskStartTime > 15.0f)
    {
        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
        if (MC) MC->MaxSpeed = 400.0f;
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    }
}

void UBTTask_FleeFromDanger::FleeUsingSteering(UBehaviorTreeComponent& OwnerComp, AActor* Threat)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return;
    }
    AIController->StopMovement();
    
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
    FleeBehavior->MaxForce = 1200.0f;
    FleeBehavior->PanicDistance = FleeDistance;
    FleeBehavior->MaxFleeDistance = FleeDistance * 1.5f;
    FleeBehavior->MaxSpeed = 700.0f;
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
AActor* UBTTask_FleeFromDanger::FindBestFleeHouse(const FVector& PawnLocation, AActor* Zombie) const
{
    if (!Zombie || !GetWorld()) return nullptr;

    const FVector ZombieLoc = Zombie->GetActorLocation();
    const FVector FleeDir = (PawnLocation - ZombieLoc).GetSafeNormal();

    AActor* BestHouse = nullptr;
    float BestScore = -FLT_MAX;

    for (TActorIterator<AHouse> It(GetWorld()); It; ++It)
    {
        AHouse* House = *It;
        const FVector HouseLoc = House->GetActorLocation();
        const float DistToHouse = FVector::Dist2D(PawnLocation, HouseLoc);
        const float HouseDistFromZombie = FVector::Dist2D(HouseLoc, ZombieLoc);

        if (DistToHouse > 4000.0f) continue;

        const FVector ToHouse = (HouseLoc - PawnLocation).GetSafeNormal();
        const float DotWithFlee = FVector::DotProduct(ToHouse, FleeDir);

        float Score = (DotWithFlee * 800.0f)
                    + HouseDistFromZombie
                    - (DistToHouse * 0.5f);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestHouse = House;
        }
    }
    return BestHouse;
}