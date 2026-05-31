#include "BTTask_FleeFromPurgeZone.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"      
#include "FleeSteering.h"           

UBTTask_FleeFromPurgeZone::UBTTask_FleeFromPurgeZone()
{
    NodeName = TEXT("Flee From Purge Zone");
    MinSafeDistance = 2500.0f;
    SearchRadius = 5000.0f;
    bUseSteeringBehavior = true; 
    
    PurgeZoneKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FleeFromPurgeZone, PurgeZoneKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FleeFromPurgeZone::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    AActor* PurgeZone = Cast<AActor>(BlackboardComp->GetValueAsObject(PurgeZoneKey.SelectedKeyName));
    
    if (!PurgeZone)
    {
        UE_LOG(LogTemp, Log, TEXT("ℹ️ [PURGE FLEE] No active purge zone"));
        BlackboardComp->SetValueAsBool(FName("InPurgeZone"), false);
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector PurgeLocation = PurgeZone->GetActorLocation();
    
    const float CurrentDistance = FVector::Dist(PawnLocation, PurgeLocation);
    
    if (CurrentDistance > MinSafeDistance)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [PURGE FLEE] Safe distance reached! (%.0fm > %.0fm)"), 
            CurrentDistance, MinSafeDistance);
        
        BlackboardComp->ClearValue(PurgeZoneKey.SelectedKeyName);
        BlackboardComp->SetValueAsBool(FName("InPurgeZone"), false);
        
        return EBTNodeResult::Succeeded;
    }

    if (bUseSteeringBehavior)
    {
        FleeUsingSteering(OwnerComp, PurgeZone);
        UE_LOG(LogTemp, Error, TEXT("🏃 [PURGE FLEE] Using STEERING to escape! Distance: %.0fm"), CurrentDistance);
        return EBTNodeResult::InProgress;
    }

    FVector FleeDirection = CalculateFleeDirection(PawnLocation, PurgeLocation);
    FVector TargetLocation = PawnLocation + (FleeDirection * MinSafeDistance * 1.5f);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [PURGE FLEE] No Navigation System!"));
        return EBTNodeResult::Failed;
    }

    FNavLocation ResultLocation;
    
    bool bFoundLocation = NavSys->ProjectPointToNavigation(
        TargetLocation,
        ResultLocation,
        FVector(SearchRadius, SearchRadius, SearchRadius)
    );

    if (!bFoundLocation)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [PURGE FLEE] Direct escape failed, trying alternatives..."));
        
        for (int32 i = 0; i < 8; i++)
        {
            float Angle = (360.0f / 8.0f) * i;
            FVector AlternativeDirection = FleeDirection.RotateAngleAxis(Angle, FVector::UpVector);
            FVector AlternativeTarget = PawnLocation + (AlternativeDirection * MinSafeDistance);
            
            if (NavSys->ProjectPointToNavigation(AlternativeTarget, ResultLocation, FVector(1000.0f)))
            {
                if (IsLocationSafeFromPurge(ResultLocation.Location, PurgeZone))
                {
                    bFoundLocation = true;
                    UE_LOG(LogTemp, Warning, TEXT("✅ [PURGE FLEE] Alternative route found at angle %.0f"), Angle);
                    break;
                }
            }
        }
    }

    if (bFoundLocation)
    {
        EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
            ResultLocation.Location,
            100.0f,
            true,
            true,
            false,
            true,
            nullptr,
            false
        );
        
        if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
        {
            UE_LOG(LogTemp, Error, TEXT("🏃 [PURGE FLEE] ESCAPING! Distance: %.0fm"), CurrentDistance);
            return EBTNodeResult::InProgress;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("☠️ [PURGE FLEE] CRITICAL! No safe path, moving randomly!"));
    
    FNavLocation RandomLocation;
    if (NavSys->GetRandomPointInNavigableRadius(PawnLocation, SearchRadius, RandomLocation))
    {
        AIController->MoveToLocation(RandomLocation.Location);
        return EBTNodeResult::InProgress;
    }

    return EBTNodeResult::Failed;
}

void UBTTask_FleeFromPurgeZone::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();
    
    if (!BlackboardComp || !AIController || !AIController->GetPawn())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AActor* PurgeZone = Cast<AActor>(BlackboardComp->GetValueAsObject(PurgeZoneKey.SelectedKeyName));
    APawn* Pawn = AIController->GetPawn();

    if (!PurgeZone)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float Distance = FVector::Dist(Pawn->GetActorLocation(), PurgeZone->GetActorLocation());
    
    if (Distance > MinSafeDistance)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [PURGE FLEE STEERING] Safe distance reached!"));
        BlackboardComp->ClearValue(PurgeZoneKey.SelectedKeyName);
        BlackboardComp->SetValueAsBool(FName("InPurgeZone"), false);
        
        if (bUseSteeringBehavior)
        {
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

void UBTTask_FleeFromPurgeZone::FleeUsingSteering(UBehaviorTreeComponent& OwnerComp, AActor* PurgeZone)
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
    FleeBehavior->SetThreatActor(PurgeZone);
    FleeBehavior->PanicDistance = MinSafeDistance * 2.0f; 
    FleeBehavior->MaxFleeDistance = MinSafeDistance * 3.0f;
    FleeBehavior->MaxSpeed = 700.0f; 
    FleeBehavior->Weight = 3.0f; 

    SteeringComp->AddSteeringBehavior(FleeBehavior);
    SteeringComp->bAutoApplySteering = true;
    SteeringComp->MaxSpeed = 700.0f;
}

FVector UBTTask_FleeFromPurgeZone::CalculateFleeDirection(const FVector& PawnLocation, const FVector& PurgeLocation) const
{
    FVector Direction = (PawnLocation - PurgeLocation);
    Direction.Z = 0.0f;
    return Direction.GetSafeNormal();
}

bool UBTTask_FleeFromPurgeZone::IsLocationSafeFromPurge(const FVector& Location, AActor* PurgeZone) const
{
    if (!PurgeZone)
    {
        return true;
    }

    const float Distance = FVector::Dist(Location, PurgeZone->GetActorLocation());
    return (Distance >= MinSafeDistance);
}