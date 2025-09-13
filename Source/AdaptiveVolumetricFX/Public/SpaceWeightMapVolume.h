// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "SpaceWeightMapVolume.generated.h"

USTRUCT()
struct ADAPTIVEVOLUMETRICFX_API FSpaceWeightNode
{
	GENERATED_BODY()
	
	UPROPERTY()
	FVector Location;
	
	UPROPERTY()
	float Density;
	
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
	
	/**
	 * <WorldLocation, Density>
	 */
	UPROPERTY()
	TArray<FSpaceWeightNode> SparseNodes;
	
	UPROPERTY()
	TArray<float> AdjacencyMatrix;
	
	void Reset()
	{
		SparseNodes.Empty();
		AdjacencyMatrix.Empty();
	}
	
	bool IsValid() const { return SparseNodes.Num() > 0; }
	
	uint64 GetSize() const { return SparseNodes.Num() * sizeof(FSpaceWeightNode) + AdjacencyMatrix.Num() * sizeof(float); }
	
	FString ToString() const
	{
		FString Result = FString::Printf(TEXT("Sparse Nodes: %d\n"), SparseNodes.Num());
		Result += FString::Printf(TEXT("Size: %2f KB"), (double)GetSize() / 1024.0f);
		return Result;
	}
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
	
	UPROPERTY(DuplicateTransient)
	FSpaceWeightMap WeightMap;
	
public:
	ASpaceWeightMapVolume();
	
	virtual void PostInitProperties() override;
	
#if WITH_EDITOR
	UFUNCTION(Blueprintable, CallInEditor, Category = "Space Weight Map Volume")
	void BakeSpaceWeightMap();
#endif
	
	FORCEINLINE const FSpaceWeightMap& GetSpaceWeightMap() const { return WeightMap; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if WITH_EDITORONLY_DATA
UCLASS(NotBlueprintable)
class USpaceWeightMapVisualizeComponent : public UActorComponent
{
	GENERATED_BODY()
};

UCLASS(HideCategories=("BrushSettings", "HLOD", "", "Tags", "Cooking", "Replication", "Networking", "WorldPartition", "LevelInstance"), MinimalAPI)
class ASpaceWeightMapDebugVolume : public AVolume
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<USpaceWeightMapVisualizeComponent> VisualizeComponent;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Space Weight Map Volume")
	TWeakObjectPtr<ASpaceWeightMapVolume> Volume;
	
	ASpaceWeightMapDebugVolume();
};
#endif