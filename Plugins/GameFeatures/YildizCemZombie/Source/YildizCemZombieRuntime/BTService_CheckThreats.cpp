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
    const float CurrentAmmo = BlackboardComp->GetValueAsFloat(FName("CurrentAmmo"));
    const float CurrentHealth = BlackboardComp->GetValueAsFloat(FName("CurrentHealth"));

    float NearestZombieDistance = FLT_MAX;
    if (NearestZombie)
    {
        NearestZombieDistance = FVector::Dist(PawnLocation, NearestZombie->GetActorLocation());
    }

    bool bShouldFlee = ShouldFleeFromZombies(ZombieCount, NearestZombieDistance, bHasWeapon, CurrentAmmo, CurrentHealth);

    BlackboardComp->SetValueAsBool(FName("ShouldFlee"), bShouldFlee);

    if (bShouldFlee)
    {
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), true);
        
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [THREAT] FLEE MODE: Zombies=%d, Distance=%.0f, HasWeapon=%s, Ammo=%.0f, Health=%.0f"), 
            ZombieCount, 
            NearestZombieDistance, 
            bHasWeapon ? TEXT("Yes") : TEXT("No"),
            CurrentAmmo,
            CurrentHealth);
    }
    else if (ZombieCount == 0)
    {
        BlackboardComp->SetValueAsBool(FName("IsInDanger"), false);
    }
}

bool UBTService_CheckThreats::ShouldFleeFromZombies(int32 ZombieCount, float NearestZombieDistance, bool bHasWeapon, float CurrentAmmo, float CurrentHealth) const
{
    if (CurrentHealth < LowHealthThreshold && NearestZombieDistance < 1200.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("💔 [THREAT] Critical health + nearby zombie!"));
        return true;
    }

    if (!bHasWeapon && NearestZombieDistance < 1200.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔫 [THREAT] No weapon + nearby zombie!"));
        return true;
    }

    if (bHasWeapon && CurrentAmmo < LowAmmoThreshold && NearestZombieDistance < 1000.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔫 [THREAT] Low ammo!"));
        return true;
    }

    if (ZombieCount >= MaxZombiesBeforeFlee)
    {
        UE_LOG(LogTemp, Warning, TEXT("🧟 [THREAT] Too many zombies (%d >= %d)!"), 
            ZombieCount, MaxZombiesBeforeFlee);
        return true;
    }

    if (NearestZombieDistance < CriticalThreatDistance)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [THREAT] Critical distance (%.0fm < %.0fm)!"), 
            NearestZombieDistance, CriticalThreatDistance);
        return true;
    }

    return false;
}