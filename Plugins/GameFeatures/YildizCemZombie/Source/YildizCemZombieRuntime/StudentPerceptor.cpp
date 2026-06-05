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
#include "PurgeZones/PurgeZone.h"

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
            
        }
    }
    if (!OwnerPawn) return;
    
    AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
    if (!AIController)
    {
        bUseManualScan = true;
        return;
    }
    
    UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
    if (!PerceptionComp)
    {
        bUseManualScan = true;
        return;
    }
    
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
        this, 
        &UStudentPerceptor::OnPerceptionUpdated
    );
    
    bUseManualScan = false;
}

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType, 
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    TimeSinceLastScan += DeltaTime;
    if (TimeSinceLastScan >= ScanInterval)
    {
        ScanHousesAndPurgeZones();         
        if (bUseManualScan)
            ScanZombiesAndItems();         
        TimeSinceLastScan = 0.0f;
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
void UStudentPerceptor::ScanHousesAndPurgeZones()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !GetWorld()) return;

    const FVector PawnLocation = OwnerPawn->GetActorLocation();

    PerceivedHouses.Empty();
    PerceivedPurgeZones.Empty();

    for (TActorIterator<AHouse> It(GetWorld()); It; ++It)
    {
        if (FVector::Dist(PawnLocation, It->GetActorLocation()) <= ScanRadius)
            PerceivedHouses.Add(*It);
    }

    for (TActorIterator<APurgeZone> It(GetWorld()); It; ++It)
    {
        APurgeZone* Zone = *It;
        FVector Origin, BoxExtent;
        Zone->GetActorBounds(false, Origin, BoxExtent);
        const float ZoneRadius = FMath::Max(BoxExtent.X, BoxExtent.Y);
        if (FVector::Dist2D(PawnLocation, Origin) <= ZoneRadius + 500.0f)
            PerceivedPurgeZones.Add(Zone);
    }
}

void UStudentPerceptor::ScanZombiesAndItems()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !GetWorld()) return;

    const FVector PawnLocation = OwnerPawn->GetActorLocation();

    PerceivedZombies.Empty();
    PerceivedItems.Empty();

    for (TActorIterator<ABaseZombie> It(GetWorld()); It; ++It)
    {
        AActor* Zombie = *It;
        if (FVector::Dist(PawnLocation, Zombie->GetActorLocation()) <= ScanRadius
            && HasLineOfSight(PawnLocation, Zombie))
        {
            PerceivedZombies.Add(Zombie);
        }
    }

    for (TActorIterator<ABaseItem> It(GetWorld()); It; ++It)
    {
        ABaseItem* Item = *It;
        if (FVector::Dist(PawnLocation, Item->GetActorLocation()) <= ScanRadius
            && HasLineOfSight(PawnLocation, Item))
        {
            PerceivedItems.Add(Item);
        }
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
bool UStudentPerceptor::HasLineOfSight(const FVector& From, AActor* Target) const
{
    if (!Target || !GetWorld()) return false;

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Cast<AActor>(GetOwner()));
    Params.AddIgnoredActor(Target);

    return !GetWorld()->LineTraceSingleByChannel(
        HitResult, From, Target->GetActorLocation(), ECC_Visibility, Params
    );
}