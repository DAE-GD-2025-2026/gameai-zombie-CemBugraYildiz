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
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"

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
		return EBTNodeResult::Failed;
	}

	if (!HasEnoughAmmo(Pawn))
	{
		BlackboardComp->SetValueAsBool(FName("IsInDanger"), true);
		return EBTNodeResult::Failed;
	}

	UStudentPerceptor* Perceptor = Pawn->FindComponentByClass<UStudentPerceptor>();
	float LocalAttackRange = AttackRange;
	
	if (Perceptor)
	{
		
		EZombieType ZType = Perceptor->GetZombieType(Target);
		
		switch (ZType)
		{
			case EZombieType::Heavy:
				LocalAttackRange = 2000.0f;
				break;
				
			case EZombieType::Runner:
				LocalAttackRange = 1200.0f;
				break;
				
			case EZombieType::Normal:
				LocalAttackRange = 1500.0f;
				break;
				
			default:
				LocalAttackRange = 1500.0f;
				break;
		}
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetLocation);

	FVector ToTarget = (TargetLocation - Pawn->GetActorLocation()).GetSafeNormal();
	FRotator TargetRotation = ToTarget.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;
	Pawn->SetActorRotation(TargetRotation);
    
	if (Distance <= LocalAttackRange)
	{
		AIController->SetFocus(Target);
		
		USteeringComponent* SteeringComp = Pawn->FindComponentByClass<USteeringComponent>();
		if (SteeringComp)
		{
			SteeringComp->ClearAllBehaviors();
			SteeringComp->bAutoApplySteering = false;
		}
        
		
		return EBTNodeResult::InProgress;
	}
	else
	{
		if (bUseSeekSteering)
		{
			ApproachUsingSeek(OwnerComp, Target);
		}
		else
		{
			EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
				TargetLocation,
				LocalAttackRange * 0.8f
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
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Distance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());

	if (Distance > AttackRange * 2.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Distance <= AttackRange)
	{
		FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
		FRotator TargetRotation = ToTarget.Rotation();
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;
		Pawn->SetActorRotation(TargetRotation);
    
		AIController->SetFocus(Target);
    
		float CurrentTime = GetWorld()->GetTimeSeconds();
		
		if (CurrentTime - LastFireTime >= FireInterval)
		{
			if (!HasEnoughAmmo(Pawn))
			{
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

	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
    
	if (!InventoryComp)
	{
		return;
	}

	FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
	FRotator TargetRotation = ToTarget.Rotation();
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;
	Pawn->SetActorRotation(TargetRotation);

	const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
    
	int32 WeaponSlot = -1;
	AWeapon* EquippedWeapon = nullptr;
    
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i])
		{
			AWeapon* Weapon = Cast<AWeapon>(Items[i]);
			if (Weapon && Weapon->GetValue() > 0) 
			{
				WeaponSlot = i;
				EquippedWeapon = Weapon;
				break;
			}
		}
	}

	if (WeaponSlot == -1 || !EquippedWeapon)
	{
		return;
	}

	bool bUsed = InventoryComp->UseItem(WeaponSlot);
    
	if (bUsed)
	{
		FString WeaponType = EquippedWeapon->GetClass()->GetName();
		int32 RemainingAmmo = EquippedWeapon->GetValue();
		
		if (EquippedWeapon->GetValue() == 0)
			InventoryComp->RemoveItem(WeaponSlot);
	}
}

bool UBTTask_AttackTarget::HasEnoughAmmo(APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	UInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UInventoryComponent>();
    
	if (!InventoryComp)
	{
		return false;
	}

	int32 TotalAmmo = 0;
	const TArray<ABaseItem*>& Items = InventoryComp->GetInventory();
    
	for (ABaseItem* Item : Items)
	{
		if (Item)
		{
			AWeapon* Weapon = Cast<AWeapon>(Item);
			if (Weapon)
			{
				TotalAmmo += Weapon->GetValue();
			}
		}
	}

	bool bHasEnough = (TotalAmmo >= MinAmmoToFight);
	return bHasEnough;
}