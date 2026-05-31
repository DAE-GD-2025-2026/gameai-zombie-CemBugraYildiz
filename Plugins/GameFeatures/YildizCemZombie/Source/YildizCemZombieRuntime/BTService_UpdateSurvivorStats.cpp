#include "BTService_UpdateSurvivorStats.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/ActorComponent.h"
#include "UObject/UnrealType.h"


UBTService_UpdateSurvivorStats::UBTService_UpdateSurvivorStats()
{
    NodeName = TEXT("Update Survivor Stats");
    Interval = 0.5f;
    RandomDeviation = 0.1f;
}

void UBTService_UpdateSurvivorStats::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    APawn* Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        return;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    const float Health = GetSurvivorHealth(Pawn);
    const float Stamina = GetSurvivorStamina(Pawn);
    const bool bHasWeapon = HasWeapon(Pawn);
    const float Ammo = GetCurrentAmmo(Pawn);

    BlackboardComp->SetValueAsFloat(FName("CurrentHealth"), Health);
    BlackboardComp->SetValueAsFloat(FName("CurrentStamina"), Stamina);
    BlackboardComp->SetValueAsBool(FName("HasWeapon"), bHasWeapon);
    BlackboardComp->SetValueAsFloat(FName("CurrentAmmo"), Ammo);

    static float LastHealth = -1.0f;
    static float LastStamina = -1.0f;
    static float LastAmmo = -1.0f;

    if (FMath::Abs(Health - LastHealth) > 1.0f || 
        FMath::Abs(Stamina - LastStamina) > 1.0f || 
        FMath::Abs(Ammo - LastAmmo) > 0.1f)
    {
        UE_LOG(LogTemp, Log, TEXT("📊 [STATS] H:%.1f S:%.1f W:%s A:%.1f"), 
            Health, Stamina, bHasWeapon ? TEXT("Y") : TEXT("N"), Ammo);
        
        LastHealth = Health;
        LastStamina = Stamina;
        LastAmmo = Ammo;
    }

    if (Health < 30.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("❤️ [STATS] CRITICAL HEALTH: %.1f"), Health);
    }

    if (Stamina < 20.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚡ [STATS] LOW STAMINA: %.1f"), Stamina);
    }

    if (bHasWeapon && Ammo < 5.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔫 [STATS] LOW AMMO: %.1f"), Ammo);
    }
}


UActorComponent* UBTService_UpdateSurvivorStats::GetHealthComponent(APawn* Pawn) const
{
    if (!Pawn)
    {
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    Pawn->GetComponents(UActorComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        if (!Comp)
        {
            continue;
        }

        const FString CompName = Comp->GetName();
        const FString ClassName = Comp->GetClass()->GetName();

        if (CompName.Contains(TEXT("Health")) || ClassName.Contains(TEXT("Health")))
        {
            return Comp;
        }
    }

    return nullptr;
}

UActorComponent* UBTService_UpdateSurvivorStats::GetStaminaComponent(APawn* Pawn) const
{
    if (!Pawn)
    {
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    Pawn->GetComponents(UActorComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        if (!Comp)
        {
            continue;
        }

        const FString CompName = Comp->GetName();
        const FString ClassName = Comp->GetClass()->GetName();

        if (CompName.Contains(TEXT("Stamina")) || ClassName.Contains(TEXT("Stamina")))
        {
            return Comp;
        }
    }

    return nullptr;
}

UActorComponent* UBTService_UpdateSurvivorStats::GetInventoryComponent(APawn* Pawn) const
{
    if (!Pawn)
    {
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    Pawn->GetComponents(UActorComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        if (!Comp)
        {
            continue;
        }

        const FString CompName = Comp->GetName();
        const FString ClassName = Comp->GetClass()->GetName();

        if (CompName.Contains(TEXT("Inventory")) || ClassName.Contains(TEXT("Inventory")))
        {
            return Comp;
        }
    }

    return nullptr;
}

float UBTService_UpdateSurvivorStats::GetSurvivorHealth(APawn* Pawn) const
{
    UActorComponent* HealthComp = GetHealthComponent(Pawn);
    
    if (!HealthComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [STATS] HealthComponent not found!"));
        return 100.0f;
    }

    FProperty* ValueProperty = HealthComp->GetClass()->FindPropertyByName(FName("Value"));
    if (ValueProperty)
    {
        if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ValueProperty))
        {
            float Health = FloatProp->GetPropertyValue_InContainer(HealthComp);
            return Health;
        }
    }

    UFunction* GetHealthFunc = HealthComp->FindFunction(FName("GetHealth"));
    if (!GetHealthFunc)
    {
        GetHealthFunc = HealthComp->FindFunction(FName("GetCurrentHealth"));
    }
    
    if (GetHealthFunc)
    {
        struct { float ReturnValue; } Params;
        HealthComp->ProcessEvent(GetHealthFunc, &Params);
        return Params.ReturnValue;
    }

    UE_LOG(LogTemp, Warning, TEXT("⚠️ [STATS] Could not read Health, using default 100.0"));
    return 100.0f;
}

float UBTService_UpdateSurvivorStats::GetSurvivorStamina(APawn* Pawn) const
{
    UActorComponent* StaminaComp = GetStaminaComponent(Pawn);
    
    if (!StaminaComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [STATS] StaminaComponent not found!"));
        return 100.0f;
    }

    FProperty* ValueProperty = StaminaComp->GetClass()->FindPropertyByName(FName("Value"));
    if (ValueProperty)
    {
        if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ValueProperty))
        {
            float Stamina = FloatProp->GetPropertyValue_InContainer(StaminaComp);
            return Stamina;
        }
    }

    UFunction* GetStaminaFunc = StaminaComp->FindFunction(FName("GetStamina"));
    if (!GetStaminaFunc)
    {
        GetStaminaFunc = StaminaComp->FindFunction(FName("GetCurrentStamina"));
    }
    
    if (GetStaminaFunc)
    {
        struct { float ReturnValue; } Params;
        StaminaComp->ProcessEvent(GetStaminaFunc, &Params);
        return Params.ReturnValue;
    }

    return 100.0f;
}

bool UBTService_UpdateSurvivorStats::HasWeapon(APawn* Pawn) const
{
    UActorComponent* InventoryComp = GetInventoryComponent(Pawn);
    
    if (!InventoryComp)
    {
        return false;
    }

    UFunction* HasWeaponFunc = InventoryComp->FindFunction(FName("HasWeapon"));
    if (HasWeaponFunc)
    {
        struct { bool ReturnValue; } Params;
        InventoryComp->ProcessEvent(HasWeaponFunc, &Params);
        return Params.ReturnValue;
    }

    UFunction* GetWeaponFunc = InventoryComp->FindFunction(FName("GetEquippedWeapon"));
    if (!GetWeaponFunc)
    {
        GetWeaponFunc = InventoryComp->FindFunction(FName("GetCurrentWeapon"));
    }
    
    if (GetWeaponFunc)
    {
        struct { UObject* ReturnValue; } Params;
        InventoryComp->ProcessEvent(GetWeaponFunc, &Params);
        return (Params.ReturnValue != nullptr);
    }

    UFunction* HasPistolFunc = InventoryComp->FindFunction(FName("HasPistol"));
    UFunction* HasShotgunFunc = InventoryComp->FindFunction(FName("HasShotgun"));
    
    if (HasPistolFunc)
    {
        struct { bool ReturnValue; } Params;
        InventoryComp->ProcessEvent(HasPistolFunc, &Params);
        if (Params.ReturnValue) return true;
    }
    
    if (HasShotgunFunc)
    {
        struct { bool ReturnValue; } Params;
        InventoryComp->ProcessEvent(HasShotgunFunc, &Params);
        if (Params.ReturnValue) return true;
    }

    return false;
}

float UBTService_UpdateSurvivorStats::GetCurrentAmmo(APawn* Pawn) const
{
    UActorComponent* InventoryComp = GetInventoryComponent(Pawn);
    
    if (!InventoryComp)
    {
        return 0.0f;
    }

    UFunction* GetAmmoFunc = InventoryComp->FindFunction(FName("GetAmmo"));
    if (!GetAmmoFunc)
    {
        GetAmmoFunc = InventoryComp->FindFunction(FName("GetCurrentAmmo"));
    }
    
    if (GetAmmoFunc)
    {
        struct { float ReturnValue; } Params;
        InventoryComp->ProcessEvent(GetAmmoFunc, &Params);
        return Params.ReturnValue;
    }

    FProperty* ValueProperty = InventoryComp->GetClass()->FindPropertyByName(FName("Value"));
    if (ValueProperty)
    {
        if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ValueProperty))
        {
            float Ammo = FloatProp->GetPropertyValue_InContainer(InventoryComp);
            return Ammo;
        }
    }

    return 0.0f;
}