// Copyright Epic Games, Inc. All Rights Reserved.

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

                		/*
                		DrawWireSphere(
							PDI,
							Node.Location,
							FMath::Lerp(FLinearColor::Green, FLinearColor::Red, Node.Density),
							10.0f,
							24,
							SDPG_World);
                		*/
                		DrawWireStar(
                			PDI,
                			Node.Location,
                			10.0f,
                			FMath::Lerp(FLinearColor::Green, FLinearColor::Red, Node.Density),
							SDPG_World);
                	}
                	/*
                	float Distance = FVector::Distance(Node.Location, View->ViewLocation);
                	if (Distance <= 1000.0f && Distance > 200.0f)
                	{
                		DrawWireSphere(
                			PDI,
                			Node.Location,
                			FMath::Lerp(FLinearColor::Green, FLinearColor::Red, Node.Density),
                			10.0f,
                			24,
                			SDPG_World);
                	}
                	*/
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