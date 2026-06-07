#include "BTService_CheckThreats.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UBTService_CheckThreats::UBTService_CheckThreats()
{
    NodeName = TEXT("Check Threats");
    Interval = 0.3f;
    RandomDeviation = 0.1f;
    
    CriticalThreatDistance = 800.0f;
    MaxZombiesBeforeFlee = 3;
    LowHealthThreshold = 30.0f;
    LowAmmoThreshold = 5.0f;
}

void UBTService_CheckThreats::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return;
    }

    APawn* Pawn = AIController->GetPawn();
    const FVector PawnLocation = Pawn->GetActorLocation();

    const int32 ZombieCount = BlackboardComp->GetValueAsInt(FName("NearbyZombieCount"));
    AActor* NearestZombie = Cast<AActor>(BlackboardComp->GetValueAsObject(FName("NearestZombie")));
    const bool bHasWeapon = BlackboardComp->GetValueAsBool(FName("HasWeapon"));
    const int32 CurrentAmmo = BlackboardComp->GetValueAsInt(FName("WeaponAmmo"));  
    const int32 CurrentHealth = BlackboardComp->GetValueAsInt(FName("CurrentHealth"));  

    float NearestZombieDistance = FLT_MAX;
    if (NearestZombie)
    {
        NearestZombieDistance = FVector::Dist(PawnLocation, NearestZombie->GetActorLocation());
    }

    bool bShouldFlee = ShouldFleeFromZombies(ZombieCount, NearestZombieDistance, bHasWeapon, CurrentAmmo, CurrentHealth);

    BlackboardComp->SetValueAsBool(FName("ShouldFlee"), bShouldFlee);
}

bool UBTService_CheckThreats::ShouldFleeFromZombies(int32 ZombieCount, float NearestZombieDistance, bool bHasWeapon, int32 CurrentAmmo, int32 CurrentHealth) const
{
    if (bHasWeapon && CurrentAmmo > 0)
        return false;

    float EffectiveThreatDistance = bHasWeapon ? CriticalThreatDistance : 1200.0f;
    if (NearestZombieDistance < EffectiveThreatDistance)
        return true;

    if (!bHasWeapon && ZombieCount >= MaxZombiesBeforeFlee && NearestZombieDistance < 1500.0f)
        return true;

    if (CurrentHealth <= 2 && NearestZombieDistance < 1200.0f)
        return true;

    return false;
}