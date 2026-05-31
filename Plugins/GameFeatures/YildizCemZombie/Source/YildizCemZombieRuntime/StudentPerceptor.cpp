#include "StudentPerceptor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Items/ItemType.h"
#include "ExplorationMemory.h"

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
    
    if (ExplorationMemory && PerceivedHouses.Num() > 0)
    {
        ExplorationMemory->UpdateHouseClusters(PerceivedHouses);
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

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        
        if (!Actor || Actor == OwnerPawn)
        {
            continue;
        }

        const float Distance = FVector::Dist(PawnLocation, Actor->GetActorLocation());
        if (Distance > ScanRadius)
        {
            continue;
        }

        FString ClassName = Actor->GetClass()->GetName();
        
        if (IsActorType(Actor, TEXT("Zombie")))
        {
            PerceivedZombies.Add(Actor);
        }
        else if (IsActorType(Actor, TEXT("Food")) || 
                 IsActorType(Actor, TEXT("Medkit")) || 
                 IsActorType(Actor, TEXT("Pistol")) || 
                 IsActorType(Actor, TEXT("Shotgun")) || 
                 IsActorType(Actor, TEXT("Garbage")))
        {
            PerceivedItems.Add(Actor);
        }
        else if (IsActorType(Actor, TEXT("House")))
        {
            PerceivedHouses.Add(Actor);
        }
        else if (IsActorType(Actor, TEXT("PurgeZone")))
        {
            PerceivedPurgeZones.Add(Actor);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("🔍 [SCAN] Z:%d | I:%d | H:%d | P:%d"),
        PerceivedZombies.Num(), PerceivedItems.Num(), 
        PerceivedHouses.Num(), PerceivedPurgeZones.Num());
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
            EItemType Type = ClassifyItem(Actor);
            
            FString ItemTypeName;
            switch (Type)
            {
                case EItemType::Medkit: ItemTypeName = TEXT("Medkit"); break;
                case EItemType::Food: ItemTypeName = TEXT("Food"); break;
                case EItemType::Pistol: ItemTypeName = TEXT("Pistol"); break;
                case EItemType::Shotgun: ItemTypeName = TEXT("Shotgun"); break;
                case EItemType::Garbage: ItemTypeName = TEXT("Garbage"); break;
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

    AActor* NearestPurge = FindNearestPurgeZone();
    if (NearestPurge)
    {
        Blackboard->SetValueAsObject(FName("ActivePurgeZone"), NearestPurge);
        Blackboard->SetValueAsBool(FName("InPurgeZone"), true);
        UE_LOG(LogTemp, Error, TEXT("☠️ [BB] PURGE ZONE ACTIVE!"));
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
        
        if (bIsInDanger)
        {
            Blackboard->SetValueAsObject(FName("NearestZombie"), MostDangerousZombie);
            Blackboard->SetValueAsInt(FName("NearbyZombieCount"), PerceivedZombies.Num());
            UE_LOG(LogTemp, Warning, TEXT("⚠️ [BB] Threat: %s at %.0fm (Total: %d)"), 
                *MostDangerousZombie->GetName(), Distance, PerceivedZombies.Num());
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

    AActor* BestItem = FindBestItem();
    if (BestItem)
    {
        Blackboard->SetValueAsObject(FName("BestItem"), BestItem);
        UE_LOG(LogTemp, Log, TEXT("✅ [BB] BestItem: %s"), *BestItem->GetName());
    }
    else
    {
        Blackboard->ClearValue(FName("BestItem"));
    }

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

EItemType UStudentPerceptor::ClassifyItem(AActor* Item) const
{
    if (!Item)
    {
        return EItemType::Garbage;
    }

    FString ClassName = Item->GetClass()->GetName();
    
    if (ClassName.Contains(TEXT("Medkit")))
    {
        return EItemType::Medkit;
    }
    else if (ClassName.Contains(TEXT("Food")))
    {
        return EItemType::Food;
    }
    else if (ClassName.Contains(TEXT("Pistol")))
    {
        return EItemType::Pistol;
    }
    else if (ClassName.Contains(TEXT("Shotgun")))
    {
        return EItemType::Shotgun;
    }
    else if (ClassName.Contains(TEXT("Garbage")))
    {
        return EItemType::Garbage;
    }

    return EItemType::Garbage;
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
            ThreatScore = 0.8f;
            break;
        case EZombieType::Heavy:
            ThreatScore = 0.6f; 
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

    UBlackboardComponent* BB = GetBlackboard();
    if (!BB)
    {
        return 0.0f;
    }

    float Priority = 0.0f;
    
    const float Health = BB->GetValueAsFloat(FName("CurrentHealth"));
    const float Energy = BB->GetValueAsFloat(FName("CurrentStamina"));
    const bool bHasWeapon = BB->GetValueAsBool(FName("HasWeapon"));
    const int32 NearbyZombies = BB->GetValueAsInt(FName("NearbyZombieCount"));

    EItemType Type = ClassifyItem(Item);

    switch (Type)
    {
    case EItemType::Medkit:
        if (Health < 30.0f) Priority = 1000.0f;
        else if (Health < 60.0f) Priority = 500.0f;
        else Priority = 100.0f;
        break;

    case EItemType::Pistol:
        if (!bHasWeapon) Priority = 900.0f;
        else if (NearbyZombies > 2) Priority = 300.0f;
        else Priority = 50.0f;
        break;

    case EItemType::Shotgun:
        if (!bHasWeapon) Priority = 900.0f;
        else if (NearbyZombies > 2) Priority = 400.0f;
        else Priority = 50.0f;
        break;

    case EItemType::Food:
        if (Energy < 20.0f) Priority = 700.0f;
        else if (Energy < 50.0f) Priority = 400.0f;
        else Priority = 200.0f;
        break;

    case EItemType::Garbage:
        Priority = -100.0f; 
        break;

    default:
        Priority = 10.0f;
        break;
    }

    const float Distance = FVector::Dist(PawnLocation, Item->GetActorLocation());
    Priority -= (Distance * 0.1f);

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