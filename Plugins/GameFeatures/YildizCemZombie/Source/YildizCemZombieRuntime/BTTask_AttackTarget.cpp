#include "BTTask_AttackTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "SteeringComponent.h"      
#include "SeekSteering.h" 
#include "StudentPerceptor.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");
	AttackRange = 1500.0f;
	MinAmmoToFight = 3.0f;
	bUseSeekSteering = true;        
	FireInterval = 0.5f;            
    
	bNotifyTick = true;            
    
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_AttackTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
    
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [ATTACK] No target"));
		return EBTNodeResult::Failed;
	}

	if (!HasEnoughAmmo(Pawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [ATTACK] Not enough ammo! Aborting attack."));
		BlackboardComp->SetValueAsBool(FName("IsInDanger"), true);
		return EBTNodeResult::Failed;
	}

	UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
	if (Perceptor)
	{
		EZombieType ZType = Perceptor->GetZombieType(Target);
		
		switch (ZType)
		{
			case EZombieType::Heavy:
				AttackRange = 2000.0f;
				UE_LOG(LogTemp, Warning, TEXT("🛡️ [ATTACK] Heavy Zombie - Keep distance! (Range: %.0f)"), AttackRange);
				break;
				
			case EZombieType::Runner:
				AttackRange = 1200.0f;
				UE_LOG(LogTemp, Warning, TEXT("⚡ [ATTACK] Runner Zombie - Shoot early! (Range: %.0f)"), AttackRange);
				break;
				
			case EZombieType::Normal:
				AttackRange = 1500.0f;
				UE_LOG(LogTemp, Log, TEXT("🧟 [ATTACK] Normal Zombie - Standard range (Range: %.0f)"), AttackRange);
				break;
				
			default:
				AttackRange = 1500.0f;
				break;
		}
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetLocation);
    
	if (Distance <= AttackRange)
	{
		AIController->SetFocus(Target);
		
		USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
		if (SteeringComp)
		{
			SteeringComp->ClearAllBehaviors();
			SteeringComp->bAutoApplySteering = false;
		}
        
		UE_LOG(LogTemp, Warning, TEXT("🔫 [ATTACK] In range (%.1fm), firing!"), Distance);
		
		return EBTNodeResult::InProgress;
	}
	else
	{
		if (bUseSeekSteering)
		{
			ApproachUsingSeek(OwnerComp, Target);
			UE_LOG(LogTemp, Log, TEXT("🎯 [ATTACK] Seeking target (Distance: %.1f)"), Distance);
		}
		else
		{
			EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
				TargetLocation,
				AttackRange * 0.8f
			);
			
			if (MoveResult != EPathFollowingRequestResult::RequestSuccessful)
			{
				return EBTNodeResult::Failed;
			}
		}
        
		return EBTNodeResult::InProgress;
	}
}

void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	APawn* Pawn = AIController->GetPawn();

	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [ATTACK] Target lost"));
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Distance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());

	if (Distance > AttackRange * 2.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("❌ [ATTACK] Target too far (%.0fm)"), Distance);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Distance <= AttackRange)
	{
		AIController->SetFocus(Target);
		
		float CurrentTime = GetWorld()->GetTimeSeconds();
		
		if (CurrentTime - LastFireTime >= FireInterval)
		{
			if (!HasEnoughAmmo(Pawn))
			{
				UE_LOG(LogTemp, Warning, TEXT("❌ [ATTACK] Out of ammo!"));
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}

			FireWeapon(Pawn, Target);
			LastFireTime = CurrentTime;
		}
	}
	else
	{
		if (bUseSeekSteering)
		{
			USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
			if (!SteeringComp || !SteeringComp->HasActiveBehaviors())
			{
				ApproachUsingSeek(OwnerComp, Target);
			}
		}
	}
}


void UBTTask_AttackTarget::ApproachUsingSeek(UBehaviorTreeComponent& OwnerComp, AActor* Target)
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

	USeekSteering* SeekBehavior = NewObject<USeekSteering>();
	SeekBehavior->SetTargetActor(Target);
	SeekBehavior->bEnableArrival = true;
	SeekBehavior->ArrivalRadius = AttackRange * 0.8f;
	SeekBehavior->MaxSpeed = 500.0f;
	SeekBehavior->Weight = 1.0f;

	SteeringComp->AddSteeringBehavior(SeekBehavior);
	SteeringComp->bAutoApplySteering = true;
	SteeringComp->MaxSpeed = 500.0f;
}

void UBTTask_AttackTarget::FireWeapon(APawn* Pawn, AActor* Target) const
{
	if (!Pawn || !Target)
	{
		return;
	}

	UActorComponent* InventoryComp = Pawn->GetComponentByClass(UActorComponent::StaticClass());
    
	if (!InventoryComp)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ATTACK] No InventoryComponent!"));
		return;
	}

	UFunction* FireFunc = InventoryComp->FindFunction(FName("Fire"));
	if (!FireFunc)
	{
		FireFunc = InventoryComp->FindFunction(FName("Shoot"));
	}
	if (!FireFunc)
	{
		FireFunc = InventoryComp->FindFunction(FName("Attack"));
	}
    
	if (!FireFunc)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [ATTACK] No Fire/Shoot/Attack function found on InventoryComponent!"));
		return;
	}

	if (FireFunc->NumParms > 0)
	{
		struct FFireParams
		{
			AActor* Target;
		};
        
		FFireParams Params;
		Params.Target = Target;
        
		InventoryComp->ProcessEvent(FireFunc, &Params);
        
		UE_LOG(LogTemp, Warning, TEXT("💥 [ATTACK] Fired at %s (with target param)"), *Target->GetName());
	}
	else
	{
		InventoryComp->ProcessEvent(FireFunc, nullptr);
        
		UE_LOG(LogTemp, Warning, TEXT("💥 [ATTACK] Fired (auto-aim)"));
	}
}

bool UBTTask_AttackTarget::HasEnoughAmmo(APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	UActorComponent* InventoryComp = Pawn->GetComponentByClass(UActorComponent::StaticClass());
	
	if (InventoryComp)
	{
		UFunction* GetAmmoFunc = InventoryComp->FindFunction(FName("GetAmmo"));
		if (GetAmmoFunc)
		{
			struct { float ReturnValue; } Params;
			InventoryComp->ProcessEvent(GetAmmoFunc, &Params);
			
			return (Params.ReturnValue >= MinAmmoToFight);
		}

		FProperty* ValueProperty = InventoryComp->GetClass()->FindPropertyByName(FName("Value"));
		if (ValueProperty)
		{
			if (FFloatProperty* FloatProp = CastField<FFloatProperty>(ValueProperty))
			{
				float Ammo = FloatProp->GetPropertyValue_InContainer(InventoryComp);
				return (Ammo >= MinAmmoToFight);
			}
		}
	}

	return false;
}