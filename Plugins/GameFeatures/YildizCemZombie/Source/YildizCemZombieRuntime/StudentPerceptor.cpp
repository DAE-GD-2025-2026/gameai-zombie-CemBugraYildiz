#include "StudentPerceptor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "ExplorationMemory.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Zombies/BaseZombie.h"
#include "Village/House/House.h"

UStudentPerceptor::UStudentPerceptor()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;
}

void UStudentPerceptor::BeginPlay()
{
    Super::BeginPlay();
    
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn)
    {
        ExplorationMemory = OwnerPawn->FindComponentByClass<UExplorationMemory>();
        if (!ExplorationMemory)
        {
            ExplorationMemory = NewObject<UExplorationMemory>(OwnerPawn);
            ExplorationMemory->RegisterComponent();
            OwnerPawn->AddInstanceComponent(ExplorationMemory);
            
            UE_LOG(LogTemp, Warning, TEXT("✅ [EXPLORATION] ExplorationMemory component created"));
        }
    }
    if (!OwnerPawn) return;
    
    AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] No AIController, using manual scan"));
        bUseManualScan = true;
        return;
    }
    
    UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
    if (!PerceptionComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("🔧 [PERCEPTION] No PerceptionComponent, using MANUAL SCAN"));
        bUseManualScan = true;
        return;
    }
    
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
        this, 
        &UStudentPerceptor::OnPerceptionUpdated
    );
    
    bUseManualScan = false;
    UE_LOG(LogTemp, Warning, TEXT("✅ [PERCEPTION] AIPerception registered successfully"));
}

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (bUseManualScan)
    {
        TimeSinceLastScan += DeltaTime;
        
        if (TimeSinceLastScan >= ScanInterval)
        {
            ManualScan();
            TimeSinceLastScan = 0.0f;
        }
    }
    
    if (ExplorationMemory && PerceivedHouses.Num() != LastPerceivedHouseCount)
    {
        ExplorationMemory->UpdateHouseClusters(PerceivedHouses);
        LastPerceivedHouseCount = PerceivedHouses.Num();
    }
    
    if (ExplorationMemory)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastMemoryCleanupTime > 60.0f)
        {
            ExplorationMemory->CleanupOldMemories(300.0f); 
            LastMemoryCleanupTime = CurrentTime;
        }
    }
    
    UpdateBlackboard();
}

void UStudentPerceptor::ManualScan()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !GetWorld())
    {
        return;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    PerceivedZombies.Empty();
    PerceivedItems.Empty();
    PerceivedHouses.Empty();
    PerceivedPurgeZones.Empty();


    for (TActorIterator<ABaseZombie> It(GetWorld()); It; ++It)
    {
        if (FVector::Dist(PawnLocation, It->GetActorLocation()) <= ScanRadius)
            PerceivedZombies.Add(*It);
    }

    for (TActorIterator<ABaseItem> It(GetWorld()); It; ++It)
    {
        if (FVector::Dist(PawnLocation, It->GetActorLocation()) <= ScanRadius)
            PerceivedItems.Add(*It);
    }

    for (TActorIterator<AHouse> It(GetWorld()); It; ++It)
    {
        if (FVector::Dist(PawnLocation, It->GetActorLocation()) <= ScanRadius)
            PerceivedHouses.Add(*It);
    }
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor)
    {
        return;
    }

    const bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
    
    if (IsActorType(Actor, TEXT("Zombie")))
    {
        if (bSuccessfullySensed)
        {
            PerceivedZombies.AddUnique(Actor);
            EZombieType Type = ClassifyZombie(Actor);
            UE_LOG(LogTemp, Warning, TEXT("🧟 [ZOMBIE DETECTED] %s (Type: %d)"), 
                *Actor->GetName(), (int32)Type);
        }
        else
        {
            PerceivedZombies.Remove(Actor);
        }
    }
    else if (IsActorType(Actor, TEXT("Food")) || 
             IsActorType(Actor, TEXT("Medkit")) ||
             IsActorType(Actor, TEXT("Pistol")) ||
             IsActorType(Actor, TEXT("Shotgun")) ||
             IsActorType(Actor, TEXT("Garbage")))
    {
        if (bSuccessfullySensed)
        {
            PerceivedItems.AddUnique(Actor);
            EMyItemType Type = ClassifyItem(Actor);
            
            FString ItemTypeName;
            switch (Type)
            {
                case EMyItemType::Medkit: ItemTypeName = TEXT("Medkit"); break;
                case EMyItemType::Food: ItemTypeName = TEXT("Food"); break;
                case EMyItemType::Pistol: ItemTypeName = TEXT("Pistol"); break;
                case EMyItemType::Shotgun: ItemTypeName = TEXT("Shotgun"); break;
                case EMyItemType::Garbage: ItemTypeName = TEXT("Garbage"); break;
                default: ItemTypeName = TEXT("Unknown"); break;
            }
            
            UE_LOG(LogTemp, Warning, TEXT("📦 [ITEM DETECTED] %s (Type: %s)"), 
                *Actor->GetName(), *ItemTypeName);
        }
        else
        {
            PerceivedItems.Remove(Actor);
        }
    }
    else if (IsActorType(Actor, TEXT("House")))
    {
        if (bSuccessfullySensed)
        {
            PerceivedHouses.AddUnique(Actor);
            UE_LOG(LogTemp, Log, TEXT("🏠 [HOUSE DETECTED] %s"), *Actor->GetName());
        }
        else
        {
            PerceivedHouses.Remove(Actor);
        }
    }
    else if (IsActorType(Actor, TEXT("PurgeZone")))
    {
        if (bSuccessfullySensed)
        {
            PerceivedPurgeZones.AddUnique(Actor);
            UE_LOG(LogTemp, Error, TEXT("☠️ [PURGE ZONE DETECTED] %s - IMMEDIATE THREAT!"), 
                *Actor->GetName());
        }
        else
        {
            PerceivedPurgeZones.Remove(Actor);
        }
    }
}

void UStudentPerceptor::UpdateBlackboard()
{
    UBlackboardComponent* Blackboard = GetBlackboard();
    if (!Blackboard)
    {
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return;
    }

    UHealthComponent* HealthComp = OwnerPawn->FindComponentByClass<UHealthComponent>();
    if (HealthComp)
    {
        const int32 CurrentHealth = HealthComp->GetHealth();
        const int32 MaxHealth = HealthComp->GetMaxHealth();
        const float HealthPercent = ((float)CurrentHealth / MaxHealth) * 100.0f;
        
        Blackboard->SetValueAsInt(FName("CurrentHealth"), CurrentHealth);
        Blackboard->SetValueAsInt(FName("MaxHealth"), MaxHealth);
        Blackboard->SetValueAsFloat(FName("HealthPercent"), HealthPercent);
        Blackboard->SetValueAsBool(FName("IsAlive"), HealthComp->IsAlive());
    }

    UStaminaComponent* StaminaComp = OwnerPawn->FindComponentByClass<UStaminaComponent>();
    if (StaminaComp)
    {
        const float CurrentStamina = StaminaComp->GetCurrentStamina();
        const float MaxStamina = StaminaComp->GetMaxStamina();
        const float StaminaPercent = (CurrentStamina / MaxStamina) * 100.0f;
        
        Blackboard->SetValueAsFloat(FName("CurrentStamina"), CurrentStamina);
        Blackboard->SetValueAsFloat(FName("MaxStamina"), MaxStamina);
        Blackboard->SetValueAsFloat(FName("StaminaPercent"), StaminaPercent);
    }

    UInventoryComponent* InventoryComp = OwnerPawn->FindComponentByClass<UInventoryComponent>();
    if (InventoryComp)
    {
        const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
        
        bool bHasWeapon = false;
        bool bHasMedkit = false;
        bool bHasFood = false;
        int32 TotalAmmo = 0;
        
        for (ABaseItem* Item : Items)
        {
            if (Item)
            {
                EItemType Type = Item->GetItemType();
                
                if (Type == EItemType::Pistol || Type == EItemType::Shotgun)
                {
                    if (Item->GetValue() > 0)
                        bHasWeapon = true; 
                    TotalAmmo += Item->GetValue();
                }
                else if (Type == EItemType::Medkit && Item->GetValue() > 0)
                    bHasMedkit = true;
                else if (Type == EItemType::Food && Item->GetValue() > 0)
                    bHasFood = true;
            }
        }
        
        Blackboard->SetValueAsBool(FName("HasWeapon"), bHasWeapon);
        Blackboard->SetValueAsBool(FName("HasMedkit"), bHasMedkit);
        Blackboard->SetValueAsBool(FName("HasFood"), bHasFood);
        Blackboard->SetValueAsInt(FName("WeaponAmmo"), TotalAmmo);
        Blackboard->SetValueAsFloat(FName("PickupRange"), InventoryComp->GetPickupRange());
    }

    AActor* NearestPurge = FindNearestPurgeZone();
    if (NearestPurge)
    {
        Blackboard->SetValueAsObject(FName("ActivePurgeZone"), NearestPurge);
        Blackboard->SetValueAsBool(FName("InPurgeZone"), true);
    }
    else
    {
        Blackboard->ClearValue(FName("ActivePurgeZone"));
        Blackboard->SetValueAsBool(FName("InPurgeZone"), false);
    }

    AActor* MostDangerousZombie = FindMostDangerousZombie();
    const float DangerRange = 1500.0f;
    bool bIsInDanger = false;
    
    if (MostDangerousZombie)
    {
        const float Distance = FVector::Dist(
            OwnerPawn->GetActorLocation(), 
            MostDangerousZombie->GetActorLocation()
        );
        
        bIsInDanger = (Distance < DangerRange);
        Blackboard->SetValueAsInt(FName("NearbyZombieCount"), PerceivedZombies.Num());
        if (bIsInDanger)
        {
            Blackboard->SetValueAsObject(FName("NearestZombie"), MostDangerousZombie);
        }
        else
        {
            Blackboard->ClearValue(FName("NearestZombie"));
        }
    }
    else
    {
        Blackboard->ClearValue(FName("NearestZombie"));
        Blackboard->SetValueAsInt(FName("NearbyZombieCount"), 0);
    }

    Blackboard->SetValueAsBool(FName("IsInDanger"), bIsInDanger);

    bool bHasInventorySpace = false;
    if (InventoryComp)
    {
        for (ABaseItem* Slot : InventoryComp->GetInventory())
        {
            if (!Slot) { bHasInventorySpace = true; break; }
        }
    }

    AActor* BestItem = nullptr;
    if (bHasInventorySpace)
    {
        BestItem = FindBestItem();
    }
    else
    {
        for (AActor* Item : PerceivedItems)
        {
            if (!Item) continue;
            ABaseItem* BaseItem = Cast<ABaseItem>(Item);
            if (BaseItem && BaseItem->GetItemType() == EItemType::Garbage)
            {
                BestItem = Item;
                break;
            }
        }
    }
    if (BestItem)
        Blackboard->SetValueAsObject(FName("BestItem"), BestItem);
    else
        Blackboard->ClearValue(FName("BestItem"));

    AActor* NearestHouse = FindNearestHouse();
    if (NearestHouse)
    {
        Blackboard->SetValueAsObject(FName("SafeHouse"), NearestHouse);
    }
}


bool UStudentPerceptor::IsActorType(AActor* Actor, const FString& TypeName) const
{
    if (!Actor)
    {
        return false;
    }

    FString ClassName = Actor->GetClass()->GetName();
    return ClassName.Contains(TypeName);
}

EZombieType UStudentPerceptor::ClassifyZombie(AActor* Zombie) const
{
    if (!Zombie)
    {
        return EZombieType::Unknown;
    }

    FString ClassName = Zombie->GetClass()->GetName();
    
    if (ClassName.Contains(TEXT("Runner")))
    {
        return EZombieType::Runner;
    }
    else if (ClassName.Contains(TEXT("Heavy")))
    {
        return EZombieType::Heavy;
    }
    else if (ClassName.Contains(TEXT("Zombie")))
    {
        return EZombieType::Normal;
    }

    return EZombieType::Unknown;
}

EMyItemType UStudentPerceptor::ClassifyItem(AActor* Item) const
{
    if (!Item)
    {
        return EMyItemType::Unknown;
    }

    ABaseItem* BaseItem = Cast<ABaseItem>(Item);
    
    if (!BaseItem)
    {
        FString ClassName = Item->GetClass()->GetName();
        
        if (ClassName.Contains(TEXT("Medkit")))
            return EMyItemType::Medkit;
        else if (ClassName.Contains(TEXT("Food")))
            return EMyItemType::Food;
        else if (ClassName.Contains(TEXT("Pistol")))
            return EMyItemType::Pistol;
        else if (ClassName.Contains(TEXT("Shotgun")))
            return EMyItemType::Shotgun;
        else if (ClassName.Contains(TEXT("Garbage")))
            return EMyItemType::Garbage;
            
        return EMyItemType::Unknown;
    }

    switch (BaseItem->GetItemType())
    {
    case EItemType::Medkit:
        return EMyItemType::Medkit;
            
    case EItemType::Food:
        return EMyItemType::Food;
            
    case EItemType::Pistol:
        return EMyItemType::Pistol;
            
    case EItemType::Shotgun:
        return EMyItemType::Shotgun;
            
    case EItemType::Garbage:
        return EMyItemType::Garbage;
            
    default:
        return EMyItemType::Unknown;
    }
}

float UStudentPerceptor::CalculateZombieThreat(AActor* Zombie, const FVector& PawnLocation) const
{
    if (!Zombie)
    {
        return 0.0f;
    }

    float ThreatScore = 0.0f;
    
    EZombieType Type = ClassifyZombie(Zombie);
    
    switch (Type)
    {
        case EZombieType::Runner:
            ThreatScore = 0.7f;
            break;
        case EZombieType::Heavy:
            ThreatScore = 0.9f; 
            break;
        case EZombieType::Normal:
            ThreatScore = 0.5f;
            break;
        default:
            ThreatScore = 0.4f;
            break;
    }
    
    const float Distance = FVector::Dist(PawnLocation, Zombie->GetActorLocation());
    const float MaxThreatRange = 2000.0f;
    const float DistanceFactor = 1.0f - FMath::Clamp(Distance / MaxThreatRange, 0.0f, 1.0f);
    
    ThreatScore += DistanceFactor;
    
    return ThreatScore;
}

float UStudentPerceptor::CalculateItemPriority(AActor* Item, const FVector& PawnLocation) const
{
    if (!Item)
    {
        return 0.0f;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return 0.0f;
    }

    UHealthComponent* HealthComp = OwnerPawn->FindComponentByClass<UHealthComponent>();
    UStaminaComponent* StaminaComp = OwnerPawn->FindComponentByClass<UStaminaComponent>();
    UInventoryComponent* InventoryComp = OwnerPawn->FindComponentByClass<UInventoryComponent>();

    if (!HealthComp || !StaminaComp || !InventoryComp)
    {
        return 0.0f;
    }

    const int32 CurrentHealth = HealthComp->GetHealth();
    const int32 MaxHealth = HealthComp->GetMaxHealth();
    const float HealthPercent = ((float)CurrentHealth / MaxHealth) * 100.0f;

    const float CurrentStamina = StaminaComp->GetCurrentStamina();
    const float MaxStamina = StaminaComp->GetMaxStamina();
    const float StaminaPercent = (CurrentStamina / MaxStamina) * 100.0f;

    bool bHasWeapon = false;
    const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
    for (ABaseItem* InventoryItem : Items)
    {
        if (InventoryItem)
        {
            EItemType Type = InventoryItem->GetItemType();
            if (Type == EItemType::Pistol || Type == EItemType::Shotgun)
            {
                if (InventoryItem->GetValue() > 0)
                    bHasWeapon = true;
                break;
            }
        }
    }

    UBlackboardComponent* BB = GetBlackboard();
    const int32 NearbyZombies = BB ? BB->GetValueAsInt(FName("NearbyZombieCount")) : 0;

    float Priority = 0.0f;

    EMyItemType Type = ClassifyItem(Item);

    switch (Type)
    {
    case EMyItemType::Medkit:
        if (CurrentHealth <= 3) Priority = 1000.0f;      
        else if (CurrentHealth <= 6) Priority = 500.0f;  
        else Priority = 100.0f;
        break;

    case EMyItemType::Food:
        if (StaminaPercent < 20.0f) Priority = 700.0f;
        else if (StaminaPercent < 50.0f) Priority = 400.0f;
        else Priority = 200.0f;
        break;

    case EMyItemType::Pistol:
    case EMyItemType::Shotgun:
        if (!bHasWeapon) Priority = 900.0f;
        else if (NearbyZombies > 2) Priority = 300.0f;
        else Priority = 50.0f;
        break;

    case EMyItemType::Garbage:
        Priority = -100.0f;
        break;

    default:
        Priority = 10.0f;
        break;
    }

    const float Distance = FVector::Dist(PawnLocation, Item->GetActorLocation());
    Priority -= FMath::Min(Distance * 0.01f, Priority * 0.5f);

    return Priority;
}

AActor* UStudentPerceptor::FindMostDangerousZombie() const
{
    if (PerceivedZombies.Num() == 0)
    {
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    AActor* MostDangerous = nullptr;
    float HighestThreat = -1.0f;

    for (AActor* Zombie : PerceivedZombies)
    {
        if (!Zombie)
        {
            continue;
        }

        const float Threat = CalculateZombieThreat(Zombie, PawnLocation);
        if (Threat > HighestThreat)
        {
            HighestThreat = Threat;
            MostDangerous = Zombie;
        }
    }

    return MostDangerous;
}

AActor* UStudentPerceptor::FindBestItem() const
{
    if (PerceivedItems.Num() == 0)
    {
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    AActor* BestItem = nullptr;
    float HighestPriority = -FLT_MAX;

    for (AActor* Item : PerceivedItems)
    {
        if (!Item)
        {
            continue;
        }

        const float Priority = CalculateItemPriority(Item, PawnLocation);
        
        if (Priority > HighestPriority)
        {
            HighestPriority = Priority;
            BestItem = Item;
        }
    }

    return BestItem;
}

AActor* UStudentPerceptor::FindNearestHouse() const
{
    if (PerceivedHouses.Num() == 0)
    {
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    AActor* NearestHouse = nullptr;
    float MinDistance = FLT_MAX;

    for (AActor* House : PerceivedHouses)
    {
        if (!House)
        {
            continue;
        }

        const float Distance = FVector::Dist(PawnLocation, House->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestHouse = House;
        }
    }

    return NearestHouse;
}

AActor* UStudentPerceptor::FindNearestPurgeZone() const
{
    if (PerceivedPurgeZones.Num() == 0)
    {
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    AActor* NearestPurge = nullptr;
    float MinDistance = FLT_MAX;

    for (AActor* Purge : PerceivedPurgeZones)
    {
        if (!Purge)
        {
            continue;
        }

        const float Distance = FVector::Dist(PawnLocation, Purge->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestPurge = Purge;
        }
    }

    return NearestPurge;
}

AActor* UStudentPerceptor::FindNearestZombie() const
{
    if (PerceivedZombies.Num() == 0)
    {
        return nullptr;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }

    const FVector PawnLocation = OwnerPawn->GetActorLocation();
    
    AActor* NearestZombie = nullptr;
    float MinDistance = FLT_MAX;

    for (AActor* Zombie : PerceivedZombies)
    {
        if (!Zombie)
        {
            continue;
        }

        const float Distance = FVector::Dist(PawnLocation, Zombie->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestZombie = Zombie;
        }
    }

    return NearestZombie;
}

UBlackboardComponent* UStudentPerceptor::GetBlackboard() const
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        return nullptr;
    }
    
    AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
    if (AIController)
    {
        return AIController->GetBlackboardComponent();
    }
    
    return nullptr;
}