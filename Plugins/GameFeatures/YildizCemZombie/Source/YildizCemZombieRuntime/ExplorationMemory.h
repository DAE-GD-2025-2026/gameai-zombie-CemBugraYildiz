#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExplorationMemory.generated.h"

USTRUCT(BlueprintType)
struct FVisitedLocation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	float VisitTime;

	UPROPERTY()
	int32 VisitCount;

	FVisitedLocation()
		: Location(FVector::ZeroVector)
		, VisitTime(0.0f)
		, VisitCount(0)
	{}

	FVisitedLocation(const FVector& InLocation, float InTime)
		: Location(InLocation)
		, VisitTime(InTime)
		, VisitCount(1)
	{}
};

USTRUCT(BlueprintType)
struct FHouseCluster
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AActor*> Houses;

	UPROPERTY()
	FVector CenterLocation;

	UPROPERTY()
	float Radius;

	UPROPERTY()
	bool bFullyExplored;

	UPROPERTY()
	float LastVisitTime;

	FHouseCluster()
		: CenterLocation(FVector::ZeroVector)
		, Radius(0.0f)
		, bFullyExplored(false)
		, LastVisitTime(0.0f)
	{}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YILDIZCEMZOMBIERUNTIME_API UExplorationMemory : public UActorComponent
{
	GENERATED_BODY()

public:
	UExplorationMemory();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	void RecordVisitedLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool IsLocationRecentlyVisited(const FVector& Location, float TimeThreshold = 120.0f) const;

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool IsLocationVisitedRecently(const FVector& Location, float Radius = 1000.0f, float TimeThreshold = 60.0f) const;

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	FVector GetLeastVisitedAreaDirection(const FVector& CurrentLocation, float SearchRadius = 3000.0f) const;

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	void UpdateHouseClusters(const TArray<AActor*>& PerceivedHouses);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	int32 FindNearestUnexploredClusterIndex(const FVector& CurrentLocation);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool IsInsideAnyCluster(const FVector& Location, int32& OutClusterIndex);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	void MarkClusterAsExplored(int32 ClusterIndex);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	bool GetClusterInfo(int32 ClusterIndex, FVector& OutCenter, float& OutRadius, int32& OutHouseCount, bool& bIsExplored);

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	int32 GetClusterCount() const { return HouseClusters.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	void CleanupOldMemories(float MaxAge = 300.0f);

	FHouseCluster* GetClusterByIndex(int32 Index);
	const TArray<FHouseCluster>& GetHouseClusters() const { return HouseClusters; }
	
	UFUNCTION(BlueprintCallable, Category = "Exploration")
	int32 GetVisitedLocationCount() const { return VisitedLocations.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Exploration")
	int32 GetTotalVisitCount() const
	{
		int32 Total = 0;
		for (const FVisitedLocation& Loc : VisitedLocations)
		{
			Total += Loc.VisitCount;
		}
		return Total;
	}

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Exploration")
	TArray<FVisitedLocation> VisitedLocations;

	UPROPERTY(EditAnywhere, Category = "Exploration")
	int32 MaxVisitedLocations = 50;

	UPROPERTY(EditAnywhere, Category = "Exploration")
	float VisitRadius = 800.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Exploration")
	TArray<FHouseCluster> HouseClusters;

	UPROPERTY(EditAnywhere, Category = "Exploration")
	float ClusterRadius = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Exploration")
	int32 MinHousesForCluster = 2;

private:
	void BuildClustersFromHouses(const TArray<AActor*>& Houses);
	bool AreHousesInSameCluster(AActor* House1, AActor* House2) const;
	FVector CalculateClusterCenter(const TArray<AActor*>& ClusterHouses) const;
	float CalculateClusterRadius(const FVector& Center, const TArray<AActor*>& ClusterHouses) const;
};