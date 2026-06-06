#include "ExplorationMemory.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UExplorationMemory::UExplorationMemory()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	MaxVisitedLocations = 50;
	VisitRadius = 800.0f;
	ClusterRadius = 2000.0f;
	MinHousesForCluster = 1;
}

void UExplorationMemory::BeginPlay()
{
	Super::BeginPlay();
}

void UExplorationMemory::RecordVisitedLocation(const FVector& Location)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	
	for (FVisitedLocation& VisitedLoc : VisitedLocations)
	{
		const float Distance = FVector::Dist(VisitedLoc.Location, Location);
		if (Distance < VisitRadius)
		{
			VisitedLoc.VisitTime = CurrentTime;
			VisitedLoc.VisitCount++;
			return;
		}
	}

	FVisitedLocation NewLocation(Location, CurrentTime);
	VisitedLocations.Add(NewLocation);

	if (VisitedLocations.Num() > MaxVisitedLocations)
	{
		VisitedLocations.RemoveAt(0);
	}
}

bool UExplorationMemory::IsLocationRecentlyVisited(const FVector& Location, float TimeThreshold) const
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (const FVisitedLocation& VisitedLoc : VisitedLocations)
	{
		const float Distance = FVector::Dist(VisitedLoc.Location, Location);
		const float TimeSinceVisit = CurrentTime - VisitedLoc.VisitTime;

		if (Distance < VisitRadius && TimeSinceVisit < TimeThreshold)
		{
			return true;
		}
	}

	return false;
}

bool UExplorationMemory::IsLocationVisitedRecently(const FVector& Location, float Radius, float TimeThreshold) const
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (const FVisitedLocation& VisitedLoc : VisitedLocations)
	{
		const float Distance = FVector::Dist(VisitedLoc.Location, Location);
		const float TimeSinceVisit = CurrentTime - VisitedLoc.VisitTime;

		if (Distance < Radius && TimeSinceVisit < TimeThreshold)
		{
			return true;
		}
	}

	return false;
}

FVector UExplorationMemory::GetLeastVisitedAreaDirection(const FVector& CurrentLocation, float SearchRadius) const
{
	if (VisitedLocations.Num() == 0)
	{
		return FVector(
			FMath::FRandRange(-1.0f, 1.0f),
			FMath::FRandRange(-1.0f, 1.0f),
			0.0f
		).GetSafeNormal();
	}

	const int32 DirectionCount = 8;
	float MinDensity = FLT_MAX;
	FVector BestDirection = FVector::ForwardVector;

	for (int32 i = 0; i < DirectionCount; i++)
	{
		const float Angle = (360.0f / DirectionCount) * i;
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector);
		const FVector TestLocation = CurrentLocation + (Direction * SearchRadius);

		int32 Density = 0;
		for (const FVisitedLocation& VisitedLoc : VisitedLocations)
		{
			const float Distance = FVector::Dist(VisitedLoc.Location, TestLocation);
			if (Distance < SearchRadius)
			{
				Density += VisitedLoc.VisitCount; 
			}
		}

		if (Density < MinDensity)
		{
			MinDensity = Density;
			BestDirection = Direction;
		}
	}
	return BestDirection;
}

void UExplorationMemory::UpdateHouseClusters(const TArray<AActor*>& PerceivedHouses)
{
	if (PerceivedHouses.Num() < MinHousesForCluster)
	{
		return; 
	}

	BuildClustersFromHouses(PerceivedHouses);
}

void UExplorationMemory::BuildClustersFromHouses(const TArray<AActor*>& Houses)
{
	TArray<FHouseCluster> NewClusters;
	TArray<AActor*> UnassignedHouses = Houses;

	while (UnassignedHouses.Num() > 0)
	{
		AActor* SeedHouse = UnassignedHouses[0];
		UnassignedHouses.RemoveAt(0);

		FHouseCluster NewCluster;
		NewCluster.Houses.Add(SeedHouse);

		for (int32 i = UnassignedHouses.Num() - 1; i >= 0; i--)
		{
			AActor* House = UnassignedHouses[i];
			
			if (AreHousesInSameCluster(SeedHouse, House))
			{
				NewCluster.Houses.Add(House);
				UnassignedHouses.RemoveAt(i);
			}
		}

		if (NewCluster.Houses.Num() >= MinHousesForCluster)
		{
			NewCluster.CenterLocation = CalculateClusterCenter(NewCluster.Houses);
			NewCluster.Radius = CalculateClusterRadius(NewCluster.CenterLocation, NewCluster.Houses);
			NewCluster.bFullyExplored = false;
			NewCluster.LastVisitTime = 0.0f;

			for (const FHouseCluster& OldCluster : HouseClusters)
			{
				const float Distance = FVector::Dist(OldCluster.CenterLocation, NewCluster.CenterLocation);
				if (Distance < ClusterRadius * 0.5f) 
				{
					NewCluster.bFullyExplored = OldCluster.bFullyExplored;
					NewCluster.LastVisitTime = OldCluster.LastVisitTime;
					break;
				}
			}
			NewClusters.Add(NewCluster);
		}
	}
	HouseClusters = NewClusters;
}

bool UExplorationMemory::AreHousesInSameCluster(AActor* House1, AActor* House2) const
{
	if (!House1 || !House2)
	{
		return false;
	}

	const float Distance = FVector::Dist(House1->GetActorLocation(), House2->GetActorLocation());
	return (Distance <= ClusterRadius);
}

FVector UExplorationMemory::CalculateClusterCenter(const TArray<AActor*>& ClusterHouses) const
{
	FVector Sum = FVector::ZeroVector;
	
	for (AActor* House : ClusterHouses)
	{
		if (House)
		{
			Sum += House->GetActorLocation();
		}
	}

	return Sum / FMath::Max(ClusterHouses.Num(), 1);
}

float UExplorationMemory::CalculateClusterRadius(const FVector& Center, const TArray<AActor*>& ClusterHouses) const
{
	float MaxDistance = 0.0f;

	for (AActor* House : ClusterHouses)
	{
		if (House)
		{
			const float Distance = FVector::Dist(Center, House->GetActorLocation());
			MaxDistance = FMath::Max(MaxDistance, Distance);
		}
	}

	return MaxDistance + 500.0f;
}

int32 UExplorationMemory::FindNearestUnexploredClusterIndex(const FVector& CurrentLocation)
{
	int32 NearestIndex = -1;
	float MinDistance = FLT_MAX;

	for (int32 i = 0; i < HouseClusters.Num(); i++)
	{
		const FHouseCluster& Cluster = HouseClusters[i];
		
		if (Cluster.bFullyExplored)
		{
			if (GetWorld()->GetTimeSeconds() - HouseClusters[i].LastVisitTime > 60.0f)
				HouseClusters[i].bFullyExplored = false;
			else
				continue;
		}

		const float Distance = FVector::Dist(CurrentLocation, Cluster.CenterLocation);
		
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			NearestIndex = i;
		}
	}
	return NearestIndex;
}

bool UExplorationMemory::IsInsideAnyCluster(const FVector& Location, int32& OutClusterIndex)
{
	for (int32 i = 0; i < HouseClusters.Num(); i++)
	{
		FHouseCluster& Cluster = HouseClusters[i];
		const float Distance = FVector::Dist(Location, Cluster.CenterLocation);
		
		if (Distance <= Cluster.Radius)
		{
			OutClusterIndex = i;
			
			Cluster.LastVisitTime = GetWorld()->GetTimeSeconds();
			
			return true;
		}
	}

	OutClusterIndex = -1;
	return false;
}

void UExplorationMemory::MarkClusterAsExplored(int32 ClusterIndex)
{
	if (ClusterIndex >= 0 && ClusterIndex < HouseClusters.Num())
	{
		HouseClusters[ClusterIndex].bFullyExplored = true;
		HouseClusters[ClusterIndex].LastVisitTime = GetWorld()->GetTimeSeconds();
	}
}

bool UExplorationMemory::GetClusterInfo(int32 ClusterIndex, FVector& OutCenter, float& OutRadius, int32& OutHouseCount, bool& bIsExplored)
{
	if (ClusterIndex >= 0 && ClusterIndex < HouseClusters.Num())
	{
		const FHouseCluster& Cluster = HouseClusters[ClusterIndex];
		OutCenter = Cluster.CenterLocation;
		OutRadius = Cluster.Radius;
		OutHouseCount = Cluster.Houses.Num();
		bIsExplored = Cluster.bFullyExplored;
		return true;
	}

	return false;
}

FHouseCluster* UExplorationMemory::GetClusterByIndex(int32 Index)
{
	if (Index >= 0 && Index < HouseClusters.Num())
	{
		return &HouseClusters[Index];
	}
	return nullptr;
}
void UExplorationMemory::CleanupOldMemories(float MaxAge)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	VisitedLocations.RemoveAll([CurrentTime, MaxAge](const FVisitedLocation& Loc)
	{
		return (CurrentTime - Loc.VisitTime) > MaxAge;
	});
}
void UExplorationMemory::MarkHouseVisited(AActor* House)
{
	if (!House || !GetWorld()) return;
	VisitedHouseMap.Add(House->GetUniqueID(), GetWorld()->GetTimeSeconds());
}

bool UExplorationMemory::IsHouseVisitedRecently(AActor* House, float TimeThreshold) const
{
	if (!House || !GetWorld()) return false;
	const float* VisitTime = VisitedHouseMap.Find(House->GetUniqueID());
	if (!VisitTime) return false;
	return (GetWorld()->GetTimeSeconds() - *VisitTime) < TimeThreshold;
}
void UExplorationMemory::RecordKnownItem(AActor* Item)
{
	if (!Item || !IsValid(Item)) return;
	KnownWorldItems.AddUnique(Item);
}

void UExplorationMemory::CleanupInvalidItems()
{
	KnownWorldItems.RemoveAll([](AActor* Item)
	{
		return !Item || !IsValid(Item);
	});
}