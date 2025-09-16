/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "SpaceWeightMapVolume.h"

#include "Components/BrushComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "AdaptiveVolumetricFX"

FString FSpaceWeightMap::ToString() const
{
	FString Result = FString::Printf(TEXT("Sparse Nodes: %d\n"), SparseNodes.Num());
	Result += FString::Printf(TEXT("Size: %2f KB\n"), (double)GetSize() / 1024.0f);
	return Result;
}

int32 FSpaceWeightMap::GetNearestNode(const FVector& QueryPoint) const
{
	int32 Index = INDEX_NONE;
	float NearestDistance = UE_MAX_FLT;
	for (int32 i = 0; i < SparseNodes.Num(); i++)
	{
		float Distance = FVector::Distance(SparseNodes[i].Location, QueryPoint);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Index = i;
		}
	}
	return Index;
}

ASpaceWeightMapVolume::ASpaceWeightMapVolume()
{
	bEnableAutoLODGeneration = false;
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
	
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	
	NodeWidth = 100.0f;
}

void ASpaceWeightMapVolume::PostInitProperties()
{
	Super::PostInitProperties();
	
#if WITH_EDITORONLY_DATA
	//if (UCubeBuilder* CubeBuilder = Cast<UCubeBuilder>(BrushBuilder))
	{
		//CubeBuilder->X = 100.0f;
		//CubeBuilder->Y = 100.0f;
		//CubeBuilder->Z = 100.0f;
	}
#endif
}

#if WITH_EDITOR
void ASpaceWeightMapVolume::BakeSpaceWeightMap()
{
	auto EvaluateNodeDensity = [this](const FVector& ParentNodeLocation, const uint32& SubDivision, const TArray<AActor*>& IgnoreActors)->float
	{
		float Density = 0.0f;
		uint32 SubNodeCount = SubDivision * SubDivision * SubDivision;
		float DensityPerSubNode = 1.0f / SubNodeCount;
		float SubNodeWidth = NodeWidth / SubDivision;
		uint32 F = SubDivision % 2 == 0 ? SubDivision / 2 : (SubDivision - 1) / 2;
		for (uint32 x = 0; x < SubDivision; x++)
		{
			for (uint32 y = 0; y < SubDivision; y++)
			{
				for (uint32 z = 0; z < SubDivision; z++)
				{
					FVector SubNodeLocation = ParentNodeLocation;
					SubNodeLocation -= FVector(F * SubNodeWidth);
					SubNodeLocation += FVector(x * SubNodeWidth, y * SubNodeWidth, z * SubNodeWidth);
					FHitResult HitResult;
					if (UKismetSystemLibrary::BoxTraceSingleForObjects(
						this,
						SubNodeLocation,
						SubNodeLocation,
						FVector(SubNodeWidth / 2.0f),
						FRotator::ZeroRotator,
						BlockNodeObjectTypes,
						false,
						IgnoreActors,
						EDrawDebugTrace::None,
						HitResult,
						true))
					{
						Density += DensityPerSubNode;
					}
				}
			}
		}
		return Density;
	};
	
	if (GEditor)
	{
		GEditor->BeginTransaction(LOCTEXT("BakeSpaceWeightMapAction", "Bake Space Weight Map"));
	}
	
	Modify();
	WeightMap.Reset();
	
	const FBoxSphereBounds VolumeBounds = GetBounds();
	// Todo
	FVector Offset = VolumeBounds.BoxExtent;
	Offset.Z = 0.0f;
	Offset -= FVector(NodeWidth / 2.0f, NodeWidth / 2.0f, 0.0f);
	
	TArray<AActor*> IgnoreActors;
	for (TActorIterator<ASpaceWeightMapVolume> It(GetWorld()); It; ++It)
	{
		if (*It)
		{
			IgnoreActors.Add(*It);
		}
	}
	for (TActorIterator<ASpaceWeightMapDebugVolume> It(GetWorld()); It; ++It)
	{
		if (*It)
		{
			IgnoreActors.Add(*It);
		}
	}
	
	{
		uint32 XCount = VolumeBounds.BoxExtent.X * 2.0f / NodeWidth;
		uint32 YCount = VolumeBounds.BoxExtent.Y * 2.0f / NodeWidth;
		uint32 ZCount = VolumeBounds.BoxExtent.Z * 2.0f / NodeWidth;
		uint64 EvaluateNodeNum = XCount * YCount * ZCount;
		
		FScopedSlowTask BakeTask(EvaluateNodeNum, LOCTEXT("BakeSpaceWeightMap_Title", "空间权重图烘焙中..."));
        BakeTask.MakeDialog();
        int32 Count = 0;
        for (uint32 x = 0; x < XCount; x++)
        {
        	for (uint32 y = 0; y < YCount; y++)
        	{
        		for (uint32 z = 0; z < ZCount; z++)
        		{
        			BakeTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("BakeSpaceWeightMap_Progress", "空间权重图烘焙中: {0} / {1}"), Count, EvaluateNodeNum));
        			FVector NodeLocation = FVector(x * NodeWidth, y * NodeWidth, z * NodeWidth) - Offset;
        			FHitResult HitResult;
        			if (UKismetSystemLibrary::BoxTraceSingleForObjects(
        				this,
        				NodeLocation,
        				NodeLocation,
        				FVector(NodeWidth / 2.0f),
        				FRotator::ZeroRotator,
        				BlockNodeObjectTypes,
        				false,
        				IgnoreActors,
        				EDrawDebugTrace::None,
        				HitResult,
        				true))
        			{
        				float Density = EvaluateNodeDensity(NodeLocation, 10, IgnoreActors);
        				if (!FMath::IsNearlyEqual(Density , 1.0f, UE_KINDA_SMALL_NUMBER))
        				{
        					WeightMap.SparseNodes.Add(FSpaceWeightNode(NodeLocation, Density));
        				}
        			}
        			else
        			{
        				WeightMap.SparseNodes.Add(FSpaceWeightNode(NodeLocation, 0.0f));
        			}
        			Count++;
        		}
        	}
        }
        BakeTask.EnterProgressFrame(0.0f, FText::Format(LOCTEXT("BakeSpaceWeightMap_Progress", "空间权重图烘焙中: {0} / {1}"), EvaluateNodeNum, EvaluateNodeNum));
	}
	{
		auto GetDirectionVector = [](const uint8& EnumBit)->FVector
		{
			switch (EnumBit)
			{
				case 0:
					return FVector(1.0f, 0.0f, 0.0f);
				case 1:
					return FVector(0.0f, 1.0f, 0.0f);
				case 2:
					return FVector(-1.0f, 0.0f, 0.0f);
				case 3:
					return FVector(0.0f, -1.0f, 0.0f);
				case 4:
					return FVector(0.0f, 0.0f, 1.0f);
				case 5:
					return FVector(0.0f, 0.0f, -1.0f);
				default:
					check(false);
					return FVector::ZeroVector;
			}
		};
		FScopedSlowTask LinkTask(WeightMap.SparseNodes.Num() + 1, LOCTEXT("LinkAdjacentNode_Title", "连接邻接节点..."));
		LinkTask.MakeDialog();
		LinkTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("LinkAdjacentNode_Progress", "连接邻接节点: {0} / {1}"), 0, WeightMap.SparseNodes.Num() + 1));
		for (int32 i = 0; i < WeightMap.SparseNodes.Num(); i++)
		{
			LinkTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("LinkAdjacentNode_Progress", "连接邻接节点: {0} / {1}"), i + 1, WeightMap.SparseNodes.Num() + 1));
			for (uint8 Direction = 0; Direction < 6; Direction++)
			{
				FVector DirectionVector = GetDirectionVector(Direction);
				FVector AdjacencyNodeLocation = WeightMap.SparseNodes[i].Location + DirectionVector * NodeWidth;
				for (int32 j = 0; j < WeightMap.SparseNodes.Num(); j++)
                {
                	if (i != j && AdjacencyNodeLocation.Equals(WeightMap.SparseNodes[j].Location))
                	{
                		FHitResult HitResult;
                		if (!UKismetSystemLibrary::LineTraceSingleForObjects(
                			this,
                			WeightMap.SparseNodes[i].Location,
                			WeightMap.SparseNodes[j].Location,
                			BlockNodeObjectTypes,
                			false,
                			IgnoreActors,
                			EDrawDebugTrace::None,
                			HitResult,
                			true))
                		{
                			WeightMap.SparseNodes[i].AdjacencyList.Add(FSpaceWeightEdge(j));
                		}
                		break;
                	}
                }
			}
		}
		LinkTask.EnterProgressFrame(0.0f, FText::Format(LOCTEXT("LinkAdjacentNode_Progress", "连接邻接节点: {0} / {1}"), WeightMap.SparseNodes.Num() + 1, WeightMap.SparseNodes.Num() + 1));
	}
	
	if (GEditor)
	{
		GEditor->EndTransaction();
	}
}
#endif

ASpaceWeightMapDebugVolume::ASpaceWeightMapDebugVolume()
{
	bEnableAutoLODGeneration = false;
	bIsEditorOnlyActor = true;
#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif
	
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	
#if WITH_EDITORONLY_DATA
	VisualizeComponent = CreateDefaultSubobject<USpaceWeightMapVisualizeComponent>(TEXT("VisualizeComponent"));
#endif
}