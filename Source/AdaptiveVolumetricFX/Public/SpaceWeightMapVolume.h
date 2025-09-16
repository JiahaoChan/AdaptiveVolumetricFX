/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"

#include "SpaceWeightMapVolume.generated.h"

USTRUCT()
struct FSpaceWeightEdge
{
	GENERATED_BODY()
	
	UPROPERTY()
	int32 Index;
	
	FSpaceWeightEdge()
	{
		Index = INDEX_NONE;
	}
	
	FSpaceWeightEdge(const int32& InIndex)
	{
		Index = InIndex;
	}
};

USTRUCT()
struct FSpaceWeightNode
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location;
	
	UPROPERTY()
	float Density;
	
	UPROPERTY()
	TArray<FSpaceWeightEdge> AdjacencyList;
	
	FSpaceWeightNode()
	{
		Location = FVector::ZeroVector;
		Density = 0.0f;
	}
	
	FSpaceWeightNode(const FVector& InLocation, const float& InDensity)
	{
		Location = InLocation;
		Density = InDensity;
	}
};

USTRUCT()
struct ADAPTIVEVOLUMETRICFX_API FSpaceWeightMap
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FSpaceWeightNode> SparseNodes;
	
	void Reset()
	{
		SparseNodes.Empty();
	}
	
	FORCEINLINE bool IsValid() const { return SparseNodes.Num() > 0; }
	FORCEINLINE uint64 GetSize() const { return SparseNodes.Num() * sizeof(FSpaceWeightNode); }
	
	FString ToString() const;
	
	int32 GetNearestNode(const FVector& QueryPoint) const;
};

UCLASS(HideCategories=("HLOD", "", "Tags", "Cooking", "Replication", "Networking", "WorldPartition", "LevelInstance"))
class ADAPTIVEVOLUMETRICFX_API ASpaceWeightMapVolume : public AVolume
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Space Weight Map Volume")
	TArray<TEnumAsByte<EObjectTypeQuery>> BlockNodeObjectTypes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Space Weight Map Volume")
	float NodeWidth;
	
	UPROPERTY(NonPIEDuplicateTransient)
	FSpaceWeightMap WeightMap;
		
public:
	ASpaceWeightMapVolume();
	
	virtual void PostInitProperties() override;
	
#if WITH_EDITOR
	UFUNCTION(Blueprintable, CallInEditor, Category = "Space Weight Map Volume")
	void BakeSpaceWeightMap();
#endif
	
	FORCEINLINE const float& GetNodeWidth() const { return NodeWidth; }
	FORCEINLINE const FSpaceWeightMap& GetSpaceWeightMap() const { return WeightMap; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if WITH_EDITORONLY_DATA
UCLASS(NotBlueprintable)
class USpaceWeightMapVisualizeComponent : public UActorComponent
{
	GENERATED_BODY()
};
#endif

UCLASS(HideCategories=("BrushSettings", "HLOD", "Collision", "Tags", "Cooking", "Replication", "Networking", "WorldPartition", "LevelInstance", "Actor", "DataLayers"), MinimalAPI)
class ASpaceWeightMapDebugVolume : public AVolume
{
	GENERATED_BODY()
	
protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<USpaceWeightMapVisualizeComponent> VisualizeComponent;
#endif
	
public:
	ASpaceWeightMapDebugVolume();
};