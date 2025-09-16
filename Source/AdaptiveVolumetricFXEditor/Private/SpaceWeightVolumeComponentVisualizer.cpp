/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "SpaceWeightVolumeComponentVisualizer.h"

#include "CanvasTypes.h"

#include "SpaceWeightMapVolume.h"

void FSpaceWeightVolumeComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const USpaceWeightMapVisualizeComponent* VisualizeComponent = Cast<const USpaceWeightMapVisualizeComponent>(Component))
	{
		if (const ASpaceWeightMapDebugVolume* DebugVolume = VisualizeComponent->GetOwner<ASpaceWeightMapDebugVolume>())
		{
			if (DebugVolume->Volume.IsValid())
			{
				const FSpaceWeightMap& SpaceWeightMap = DebugVolume->Volume->GetSpaceWeightMap();
                for (const FSpaceWeightNode& Node : SpaceWeightMap.SparseNodes)
                {
                	if (DebugVolume->EncompassesPoint(Node.Location))
                	{
                		DrawWireSphere(
							PDI,
							Node.Location,
							FMath::Lerp(FLinearColor::Green, FLinearColor::Red, Node.Density),
							5.0f,
							16,
							SDPG_World,
							1.0f);
                		for (const FSpaceWeightEdge& Edge : Node.AdjacencyList)
                		{
                			if (!SpaceWeightMap.SparseNodes.IsValidIndex(Edge.Index) || !DebugVolume->EncompassesPoint(SpaceWeightMap.SparseNodes[Edge.Index].Location))
                			{
                				continue;
                			}
                			FTransform EdgeTransform = FTransform::Identity;
                			FVector Direction = SpaceWeightMap.SparseNodes[Edge.Index].Location - Node.Location;
                			Direction.Normalize();
                			EdgeTransform.SetRotation(Direction.ToOrientationQuat());
                			FVector OffsetDirection = FVector(Direction.Z, Direction.X, Direction.Y);
                			EdgeTransform.SetLocation(Node.Location + Direction * DebugVolume->Volume->GetNodeWidth() * 0.1f + OffsetDirection * 4.0f);
                			DrawDirectionalArrow(
                				PDI,
                				EdgeTransform.ToMatrixNoScale(),
                				FLinearColor(0.1f, 0.1f, 0.1f),
                				DebugVolume->Volume->GetNodeWidth() * 0.8f,
                				4.0f,
                				SDPG_Foreground);
                		}
                	}
                }
			}
		}
	}
}

void FSpaceWeightVolumeComponentVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	if (const USpaceWeightMapVisualizeComponent* VisualizeComponent = Cast<const USpaceWeightMapVisualizeComponent>(Component))
	{
		if (const ASpaceWeightMapDebugVolume* DebugVolume = VisualizeComponent->GetOwner<ASpaceWeightMapDebugVolume>())
		{
			if (DebugVolume->Volume.IsValid())
			{
				FString DebugString = DebugVolume->Volume->GetActorLabel() + FString("\n") + DebugVolume->Volume->GetSpaceWeightMap().ToString();
				Canvas->DrawShadowedText(45.0f, 45.0f, FText::FromString(DebugString), GEditor->GetLargeFont(), FLinearColor::Green);
			}
		}
	}
}