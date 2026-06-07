#include "BTTask_FleeFromDanger.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"     
#include "FleeSteering.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "SeekSteering.h"
#include "ExplorationMemory.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_FleeFromDanger::UBTTask_FleeFromDanger()
{
    NodeName = TEXT("Flee From Danger (Zombies)");
    FleeDistance = 1500.0f;
    SearchRadius = 3000.0f;
    bPreferHousesWhenFleeing = false;
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
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();
    const FVector DangerLocation = DangerActor->GetActorLocation();
    const float CurrentDistance = FVector::Dist(PawnLocation, DangerLocation);
    
    FFleeTaskMemory* Memory = CastInstanceNodeMemory<FFleeTaskMemory>(NodeMemory);
    Memory->TaskStartTime = GetWorld()->GetTimeSeconds();
    Memory->bSeekingWeapon = false;
    Memory->TargetWeapon = nullptr; 
    
    const float SafeDistance = FleeDistance;
    if (CurrentDistance > SafeDistance)
    {
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false);
        return EBTNodeResult::Succeeded;
    }

    AIController->StopMovement();
    UFloatingPawnMovement* MoveComp = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
    if (MoveComp) MoveComp->MaxSpeed = 600.0f;
    
    if (bUseSteeringBehavior)
    {
        if (!BlackboardComp->GetValueAsBool(FName("HasWeapon")))
        {
            AActor* KnownWeapon = FindNearestKnownWeapon(Pawn, DangerActor);
            if (KnownWeapon)
            {
                Memory->TargetWeapon  = KnownWeapon;
                Memory->bSeekingWeapon = true;
                AIController->MoveToActor(KnownWeapon, FleeWeaponGrabRadius * 0.8f);
                return EBTNodeResult::InProgress;
            }
        }
        FleeUsingSteering(OwnerComp, DangerActor);
        return EBTNodeResult::InProgress;
    }
    
    if (!BlackboardComp->GetValueAsBool(FName("HasWeapon")))
    {
        AActor* KnownWeapon = FindNearestKnownWeapon(Pawn, DangerActor);
        if (KnownWeapon)
        {
            UNavigationSystemV1* NavSysW = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            FNavLocation WeaponNav;
            if (NavSysW && NavSysW->ProjectPointToNavigation(
                    KnownWeapon->GetActorLocation(), WeaponNav, FVector(500.f, 500.f, 200.f)))
            {
                EPathFollowingRequestResult::Type Res =
                    AIController->MoveToLocation(WeaponNav.Location, 100.0f);
                if (Res == EPathFollowingRequestResult::RequestSuccessful)
                    return EBTNodeResult::InProgress;
            }
        }
    }
    
    const FVector FleeDirection = (PawnLocation - DangerLocation).GetSafeNormal();
    FVector TargetLocation = PawnLocation + (FleeDirection * FleeDistance);

    if (bPreferHousesWhenFleeing)
    {
        AActor* SafeHouse = FindNearestSafeHouse(BlackboardComp, PawnLocation, DangerActor);
        if (SafeHouse)
        {
            float HouseDistanceFromZombie = FVector::Dist(SafeHouse->GetActorLocation(), DangerLocation);
            if (HouseDistanceFromZombie > CurrentDistance)
                TargetLocation = SafeHouse->GetActorLocation();
        }
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
        return EBTNodeResult::Failed;

    const FVector PrimaryDir = (TargetLocation - PawnLocation).GetSafeNormal();

    const FVector FleeDirections[] = {
        PrimaryDir,
        FleeDirection.RotateAngleAxis( 45.0f, FVector::UpVector),
        FleeDirection.RotateAngleAxis(-45.0f, FVector::UpVector),
        FleeDirection.RotateAngleAxis( 90.0f, FVector::UpVector),
    };

    for (const FVector& Dir : FleeDirections)
    {
        FVector TryTarget = PawnLocation + Dir * FleeDistance;
        FNavLocation NavLoc;

        if (!NavSys->ProjectPointToNavigation(TryTarget, NavLoc, FVector(500.0f, 500.0f, 200.0f)))
            continue;

        if (FVector::Dist2D(NavLoc.Location, PawnLocation) < 300.0f)
            continue;

        EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(NavLoc.Location, 100.0f);
        if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
            return EBTNodeResult::InProgress;
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
    
    if (Memory->bSeekingWeapon)
    {
        AActor* WeaponActor = Memory->TargetWeapon.Get();

        if (!WeaponActor || !IsValid(WeaponActor))
         {
            Memory->bSeekingWeapon = false;
            AIController->StopMovement();
            FleeUsingSteering(OwnerComp, DangerActor);
            return;
         }

        const FVector PawnLocW  = Pawn->GetActorLocation();
        const float DistToWeapon = FVector::Dist2D(PawnLocW, WeaponActor->GetActorLocation());
        const float ZombieDist   = FVector::Dist(PawnLocW, DangerActor->GetActorLocation());

        if (ZombieDist < FleeDistance * 0.35f)
        {
            Memory->bSeekingWeapon = false;
            AIController->StopMovement();
            FleeUsingSteering(OwnerComp, DangerActor);
            return;
        }
        if (DistToWeapon <= FleeWeaponGrabRadius)
        {
            UInventoryComponent* InvComp = Pawn->FindComponentByClass<UInventoryComponent>();
            ABaseItem* WeaponItem = Cast<ABaseItem>(WeaponActor);
            if (InvComp && WeaponItem)
            {
                int32 EmptySlot = -1;
                for (int32 i = 0; i < InvComp->GetInventory().Num(); i++)
                    if (!InvComp->GetInventory()[i]) { EmptySlot = i; break; }
    
                if (EmptySlot >= 0 && InvComp->GrabItem(EmptySlot, WeaponItem))
                {
                    BB->SetValueAsBool(FName("HasWeapon"), true);
                    AIController->StopMovement();
                    UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
                    if (MC) MC->MaxSpeed = 400.0f;
                    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
                    return;
                }
            }
            Memory->bSeekingWeapon = false;
            AIController->StopMovement();
            FleeUsingSteering(OwnerComp, DangerActor);
            return;
        }
    
        if (GetWorld()->GetTimeSeconds() - Memory->TaskStartTime > 15.0f)
        {
            Memory->bSeekingWeapon = false;
            AIController->StopMovement();
            FleeUsingSteering(OwnerComp, DangerActor);
            return;
        }
    
        return;
    }

    if (!BB->GetValueAsBool(FName("HasWeapon")))
    {
        UExplorationMemory* ExpMem = Pawn->FindComponentByClass<UExplorationMemory>();
        UInventoryComponent* InvComp = Pawn->FindComponentByClass<UInventoryComponent>();

        if (ExpMem && InvComp)
        {
            int32 EmptySlot = -1;
            const TArray<ABaseItem*>& Inv = InvComp->GetInventory();
            for (int32 i = 0; i < Inv.Num(); i++)
                if (!Inv[i]) { EmptySlot = i; break; }

            if (EmptySlot != -1)
            {
                const FVector PawnLoc = Pawn->GetActorLocation();
                for (AActor* ItemActor : ExpMem->GetKnownWorldItems())
                {
                    if (!ItemActor || !IsValid(ItemActor)) continue;
                    ABaseItem* Item = Cast<ABaseItem>(ItemActor);
                    if (!Item) continue;
                    EItemType Type = Item->GetItemType();
                    if (Type != EItemType::Pistol && Type != EItemType::Shotgun) continue;
                    if (Item->GetValue() <= 0) continue;

                    if (FVector::Dist2D(PawnLoc, ItemActor->GetActorLocation()) > FleeWeaponGrabRadius)
                        continue;

                    if (InvComp->GrabItem(EmptySlot, Item))
                    {
                        USteeringComponent* SC = Pawn->FindComponentByClass<USteeringComponent>();
                        if (SC) { SC->ClearAllBehaviors(); SC->bAutoApplySteering = false; }
                        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
                        if (MC) MC->MaxSpeed = 400.0f;
                        BB->SetValueAsBool(FName("IsInDanger"), false);
                        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
                        return;
                    }
                }
            }
        }
    }
    
    const FVector PawnLoc = Pawn->GetActorLocation();
    const float ZombieDist = FVector::Dist(PawnLoc, DangerActor->GetActorLocation());

    if (ZombieDist > FleeDistance)
    {
        USteeringComponent* SC = Pawn->FindComponentByClass<USteeringComponent>();
        if (SC) { SC->ClearAllBehaviors(); SC->bAutoApplySteering = false; }

        BB->SetValueAsBool(FName("IsInDanger"), false);
        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
        if (MC) MC->MaxSpeed = 400.0f;
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    if (GetWorld()->GetTimeSeconds() - Memory->TaskStartTime > 15.0f)
    {
        USteeringComponent* SC = Pawn->FindComponentByClass<USteeringComponent>();
        if (SC) { SC->ClearAllBehaviors(); SC->bAutoApplySteering = false; }

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
EBTNodeResult::Type UBTTask_FleeFromDanger::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
        UFloatingPawnMovement* MC = Cast<UFloatingPawnMovement>(Pawn->GetMovementComponent());
        if (MC) MC->MaxSpeed = 400.0f;
    }
    return Super::AbortTask(OwnerComp, NodeMemory);
}
AActor* UBTTask_FleeFromDanger::FindNearestKnownWeapon(APawn* Pawn, AActor* Threat) const
{
    UExplorationMemory* ExpMem = Pawn->FindComponentByClass<UExplorationMemory>();
    UInventoryComponent* InvComp = Pawn->FindComponentByClass<UInventoryComponent>();
    if (!ExpMem) return nullptr;

    ExpMem->CleanupInvalidItems();

    AActor* Best = nullptr;
    float MinDist = FLT_MAX;
    const FVector PawnLoc = Pawn->GetActorLocation();
    const FVector ThreatLoc = Threat ? Threat->GetActorLocation() : PawnLoc;

    for (AActor* Item : ExpMem->GetKnownWorldItems())
    {
        if (!Item || !IsValid(Item)) continue;
        ABaseItem* BaseItem = Cast<ABaseItem>(Item);
        if (!BaseItem) continue;

        EItemType Type = BaseItem->GetItemType();
        if (Type != EItemType::Pistol && Type != EItemType::Shotgun) continue;
        if (BaseItem->GetValue() <= 0) continue; 

        bool bInInv = false;
        if (InvComp)
            for (ABaseItem* InvItem : InvComp->GetInventory())
                if (InvItem == BaseItem) { bInInv = true; break; }
        if (bInInv) continue;

        if (FVector::Dist2D(Item->GetActorLocation(), ThreatLoc) < FleeDistance * 0.4f)
            continue;

        float Dist = FVector::Dist2D(PawnLoc, Item->GetActorLocation());
        if (Dist > FleeDistance * 1.2f) continue;
        
        float DistWeaponToThreat = FVector::Dist2D(Item->GetActorLocation(), ThreatLoc);
        if (Dist > DistWeaponToThreat) continue;
        if (Dist < MinDist) { MinDist = Dist; Best = Item; }
    }
    return Best;
}