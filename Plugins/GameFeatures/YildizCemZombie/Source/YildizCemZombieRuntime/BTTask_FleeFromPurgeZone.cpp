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
    bNotifyTick = true;
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
        BlackboardComp->SetValueAsBool(FName("InPurgeZone"), false);
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector PurgeLocation = PurgeZone->GetActorLocation();
    const float CurrentDistance = FVector::Dist(PawnLocation, PurgeLocation);
    
    FVector ZoneOrigin, ZoneExtent;
    PurgeZone->GetActorBounds(false, ZoneOrigin, ZoneExtent);
    const float ZoneRadius = FMath::Max(ZoneExtent.X, ZoneExtent.Y);
    const float SafeDistance = ZoneRadius + 300.0f;
    
    if (CurrentDistance > SafeDistance)
    {
        BlackboardComp->ClearValue(PurgeZoneKey.SelectedKeyName);
        BlackboardComp->SetValueAsBool(FName("InPurgeZone"), false);
        
        return EBTNodeResult::Succeeded;
    }

    if (bUseSteeringBehavior)
    {
        FleeUsingSteering(OwnerComp, PurgeZone);
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
    
    FVector ZoneOrigin, ZoneExtent;
    PurgeZone->GetActorBounds(false, ZoneOrigin, ZoneExtent);
    const float ZoneRadius = FMath::Max(ZoneExtent.X, ZoneExtent.Y);
    const float SafeDistance = ZoneRadius + 300.0f;
    
    if (Distance > SafeDistance)
    {
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
    
    FVector ZoneOrigin, ZoneExtent;
    PurgeZone->GetActorBounds(false, ZoneOrigin, ZoneExtent);
    const float ZoneRadius = FMath::Max(ZoneExtent.X, ZoneExtent.Y);

    UFleeSteering* FleeBehavior = NewObject<UFleeSteering>();
    FleeBehavior->SetThreatActor(PurgeZone);
    FleeBehavior->PanicDistance = ZoneRadius * 2.0f; 
    FleeBehavior->MaxFleeDistance = ZoneRadius * 3.0f;
    FleeBehavior->MaxForce = 1500.0f;
    FleeBehavior->MaxSpeed = 700.0f; 
    FleeBehavior->Weight = 3.0f; 

    SteeringComp->AddSteeringBehavior(FleeBehavior);
    SteeringComp->bAutoApplySteering = true;
    SteeringComp->MaxSpeed = 700.0f;
}