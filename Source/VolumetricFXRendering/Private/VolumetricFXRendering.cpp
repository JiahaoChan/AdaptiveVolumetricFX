// Copyright Epic Games, Inc. All Rights Reserved.

#include "VolumetricFXRendering.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FVolumetricFXRenderingModule"

void FVolumetricFXRenderingModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("AdaptiveVolumetricFX"))->GetBaseDir(), TEXT("Shaders/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/VolumetricFXRenderingShaders"), PluginShaderDir);
}

void FVolumetricFXRenderingModule::ShutdownModule()
{
	
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVolumetricFXRenderingModule, VolumetricFXRendering)