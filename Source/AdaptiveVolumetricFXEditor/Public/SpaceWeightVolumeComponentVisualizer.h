// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"
#include "CoreMinimal.h"

class FPrimitiveDrawInterface;
class FSceneView;
class UActorComponent;

class ADAPTIVEVOLUMETRICFXEDITOR_API FSpaceWeightVolumeComponentVisualizer : public FComponentVisualizer
{
public:
	//~ Begin FComponentVisualizer Interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	//~ End FComponentVisualizer Interface
};
