#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "StudentPerceptor.generated.h"

class UHealthComponent;
class UStaminaComponent;
class UInventoryComponent;
class ABaseItem;
class UExplorationMemory;

UENUM(BlueprintType)
enum class EZombieType : uint8
{
	Normal,
	Runner,
	Heavy,
	Unknown
};

UENUM(BlueprintType)
enum class EMyItemType : uint8
{
	Medkit,
	Food,
	Pistol,
	Shotgun,
	Garbage,
	Unknown
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YILDIZCEMZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();
    
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION(BlueprintCallable, Category = "Perception")
	TArray<AActor*> GetPerceivedZombies() const { return PerceivedZombies; }

	UFUNCTION(BlueprintCallable, Category = "Perception")
	TArray<AActor*> GetPerceivedItems() const { return PerceivedItems; }

	UFUNCTION(BlueprintCallable, Category = "Perception")
	TArray<AActor*> GetPerceivedHouses() const { return PerceivedHouses; }

	UFUNCTION(BlueprintCallable, Category = "Perception")
	EZombieType GetZombieType(AActor* Zombie) const { return ClassifyZombie(Zombie); }

	UFUNCTION(BlueprintCallable, Category = "Perception")
	UExplorationMemory* GetExplorationMemory() const { return ExplorationMemory; }
	
	void MarkItemFailed(AActor* Item);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	TArray<AActor*> PerceivedZombies;

	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	TArray<AActor*> PerceivedItems;

	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	TArray<AActor*> PerceivedHouses;

	UPROPERTY(BlueprintReadOnly, Category = "Perception")
	TArray<AActor*> PerceivedPurgeZones;

	UPROPERTY()
	UExplorationMemory* ExplorationMemory;

	UPROPERTY(EditAnywhere, Category = "Perception")
	float ScanRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Perception")
	float ScanInterval = 0.5f;

	void UpdateBlackboard();
	void ScanHousesAndPurgeZones();
	void ScanZombiesAndItems();
    
	class UBlackboardComponent* GetBlackboard() const;
    
	AActor* FindMostDangerousZombie() const;
	AActor* FindBestItem() const;
	AActor* FindNearestHouse() const;
	AActor* FindNearestPurgeZone() const;

	EZombieType ClassifyZombie(AActor* Zombie) const;
	EMyItemType ClassifyItem(AActor* Item) const;
	bool IsActorType(AActor* Actor, const FString& TypeName) const;
	
	float CalculateZombieThreat(AActor* Zombie, const FVector& PawnLocation) const;
	float CalculateItemPriority(AActor* Item, const FVector& PawnLocation) const;

private:
	float TimeSinceLastScan = 0.0f;
	bool bUseManualScan = true;
	float LastMemoryCleanupTime = 0.0f; 
	int32 LastPerceivedHouseCount = 0;
	mutable TMap<uint32, float> FailedItemCooldowns;
	bool HasLineOfSight(const FVector& From, AActor* Target) const;
};