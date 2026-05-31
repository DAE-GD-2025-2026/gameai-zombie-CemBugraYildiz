#include "BTTask_SelectWeapon.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/ActorComponent.h"

UBTTask_SelectWeapon::UBTTask_SelectWeapon()
{
    NodeName = TEXT("Select Best Weapon");
}

EBTNodeResult::Type UBTTask_SelectWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    AActor* TargetZombie = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetZombieKey.SelectedKeyName));
    const int32 NearbyZombieCount = BlackboardComp->GetValueAsInt(FName("NearbyZombieCount"));

    if (!TargetZombie)
    {
        return EBTNodeResult::Failed;
    }

    FString BestWeapon = GetBestWeaponForTarget(TargetZombie, NearbyZombieCount);
    
    EquipWeapon(AIController->GetPawn(), BestWeapon);

    UE_LOG(LogTemp, Log, TEXT("🔫 [WEAPON SELECT] Equipped: %s"), *BestWeapon);

    return EBTNodeResult::Succeeded;
}

FString UBTTask_SelectWeapon::GetBestWeaponForTarget(AActor* Zombie, int32 NearbyZombieCount) const
{
    if (!Zombie)
    {
        return TEXT("Pistol");
    }

    FString ZombieClass = Zombie->GetClass()->GetName();

    if (ZombieClass.Contains(TEXT("Heavy")))
    {
        return TEXT("Pistol");
    }

    if (NearbyZombieCount > 2)
    {
        return TEXT("Shotgun");
    }

    if (ZombieClass.Contains(TEXT("Runner")))
    {
        return TEXT("Pistol");
    }

    return TEXT("Pistol");
}

void UBTTask_SelectWeapon::EquipWeapon(APawn* Pawn, const FString& WeaponType) const
{
    if (!Pawn)
    {
        return;
    }

    UActorComponent* InventoryComp = Pawn->GetComponentByClass(UActorComponent::StaticClass());
    
    if (InventoryComp)
    {
        UFunction* EquipFunc = InventoryComp->FindFunction(FName("EquipWeapon"));
        if (!EquipFunc)
        {
            EquipFunc = InventoryComp->FindFunction(FName("SwitchWeapon"));
        }
        
        if (EquipFunc)
        {
            struct FEquipParams
            {
                FString WeaponName;
            };
            
            FEquipParams Params;
            Params.WeaponName = WeaponType;
            
            InventoryComp->ProcessEvent(EquipFunc, &Params);
        }
    }
}