/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "AdaptiveSmokeBomb.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/TextureRenderTarget2D.h"

#include "SpaceWeightMapVolume.h"
#include "VolumetricFXSDFComputeShader.h"

AAdaptiveSmokeBomb::AAdaptiveSmokeBomb()
{
	PrimaryActorTick.bCanEverTick = false;
	
	MeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	
	SpreadWeight = 10.0f;
	
	SmokeSDFTexture = nullptr;
}

void AAdaptiveSmokeBomb::BeginPlay()
{
	Super::BeginPlay();
}

void AAdaptiveSmokeBomb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAdaptiveSmokeBomb::Explode()
{
	ExplodeLayers.Reset();
	for (TActorIterator<ASpaceWeightMapVolume> It(GetWorld()); It; ++It)
	{
		if (*It)
		{
			const FSpaceWeightMap& SpaceWeightMap = (*It)->GetSpaceWeightMap();
			int32 FirstNode = SpaceWeightMap.GetNearestNode(GetActorLocation());
			if (FirstNode != INDEX_NONE && SpaceWeightMap.SparseNodes.IsValidIndex(FirstNode))
			{
				TSet<int32> AllNodes;
				AllNodes.Add(FirstNode);
				
				FExplodeLayer FirstLayer;
				FirstLayer.Nodes.Add(SpaceWeightMap.SparseNodes[FirstNode].Location);
				ExplodeLayers.Add(FirstLayer);
				
				float RemainWeight = SpreadWeight;
				TQueue<int32> NodeQueue;
				NodeQueue.Enqueue(FirstNode);
				while (RemainWeight > 0.0f)
				{
					if (NodeQueue.IsEmpty())
					{
						break;
					}
					TArray<int32> NextLayerNodes;
					while(!NodeQueue.IsEmpty())
					{
						bool bHasConsumedAllWeight = false;
						int32 CurrentNode = INDEX_NONE;
						if (NodeQueue.Dequeue(CurrentNode))
						{
							if (CurrentNode != INDEX_NONE && SpaceWeightMap.SparseNodes.IsValidIndex(CurrentNode))
							{
								for (const FSpaceWeightEdge& Edge : SpaceWeightMap.SparseNodes[CurrentNode].AdjacencyList)
                                {
                                	if (Edge.Index != INDEX_NONE && SpaceWeightMap.SparseNodes.IsValidIndex(Edge.Index))
                                	{
                                		if (!AllNodes.Contains(Edge.Index))
                                		{
                                			NextLayerNodes.Add(Edge.Index);
                                			AllNodes.Add(Edge.Index);
                                			RemainWeight -= 1.0f;
                                			if (RemainWeight <= 0.0f)
                                			{
                                				bHasConsumedAllWeight = true;
                                				break;
                                			}
                                		}
                                	}
                                }
							}
						}
						if (bHasConsumedAllWeight)
						{
							break;
						}
					}
					FExplodeLayer NextLayer;
					for (const int32 NextNode : NextLayerNodes)
					{
						NodeQueue.Enqueue(NextNode);
						NextLayer.Nodes.Add(SpaceWeightMap.SparseNodes[NextNode].Location);
					}
					if (NextLayer.Nodes.Num() > 0)
					{
						ExplodeLayers.Add(NextLayer);
					}
				}
				break;
			}
		}
	}
	
	{
		TArray<FVector> AllNodes;
		for (const FExplodeLayer& Layer : ExplodeLayers)
		{
			AllNodes.Append(Layer.Nodes);
			//for (const FVector& Node : Layer.Nodes)
			//{
				//AllNodes.Add(FVector3f(Node));
			//}
		}
		if (!SmokeSDFTexture)
		{
			SmokeSDFTexture = NewObject<UTextureRenderTarget2D>(this);
			check(SmokeSDFTexture);
			SmokeSDFTexture->RenderTargetFormat = RTF_R16f;
			SmokeSDFTexture->SRGB = false;
			SmokeSDFTexture->bCanCreateUAV = true;
			SmokeSDFTexture->AddressX = TA_Clamp;
			SmokeSDFTexture->AddressY = TA_Clamp;
			SmokeSDFTexture->InitAutoFormat(512.0f, 512.0f);
		}
		
		FVolumetircFXSDFCSParams Params;
		Params.VoxelPointLocation = AllNodes;
		Params.BoundsOrigin = GetActorLocation();
		Params.BoundsSize = 500.0f;
		Params.SDFTexture = SmokeSDFTexture;
		
		FVolumetricFXSDFComputeShaderInterface::Dispatch(Params, nullptr);
	}
}