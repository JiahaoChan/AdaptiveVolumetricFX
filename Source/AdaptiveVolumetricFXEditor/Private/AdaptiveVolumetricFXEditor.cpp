/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "AdaptiveVolumetricFXEditor.h"

#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

#include "SpaceWeightMapVolume.h"
#include "SpaceWeightVolumeComponentVisualizer.h"

#define LOCTEXT_NAMESPACE "FAdaptiveVolumetricFXEditorModule"

void FAdaptiveVolumetricFXEditorModule::StartupModule()
{
	RegisterComponentVisualizer(USpaceWeightMapVisualizeComponent::StaticClass()->GetFName(), MakeShareable(new FSpaceWeightVolumeComponentVisualizer));
}

void FAdaptiveVolumetricFXEditorModule::ShutdownModule()
{
	
}

void FAdaptiveVolumetricFXEditorModule::RegisterComponentVisualizer(FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
{
	if (GUnrealEd != NULL)
	{
		GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	}
	if (Visualizer.IsValid())
	{
		Visualizer->OnRegister();
	}
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAdaptiveVolumetricFXEditorModule, AdaptiveVolumetricFXEditor)