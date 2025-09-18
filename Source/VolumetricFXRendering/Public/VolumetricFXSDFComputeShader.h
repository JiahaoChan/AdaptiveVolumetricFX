/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "VolumetricFXSDFComputeShader.generated.h"

// Todo
USTRUCT(BlueprintType)
struct FSubductionConeShape
{
	GENERATED_BODY()
	
	FVector3f Axis;
	FVector3f Point;
	float Height;
	float BottomRadius;
	float TopRadius;
};

// Todo
USTRUCT(BlueprintType)
struct FVolumetircFXSDFCSParams
{
	GENERATED_BODY()
	
	int32 Output;
	
	// Input
	TArray<FVector3f> VoxelPointLocations;
	float BoundsSize;
	uint32 LayerResolution;
	uint32 LayerTilesCount;
	float InnerRadius;
	float OuterRadius;
	float FactorK;
	TArray<FSubductionConeShape> ConeShapes;
	
	// Output
	TObjectPtr<UTextureRenderTarget2D> SDFTexture;
	
	FVolumetircFXSDFCSParams()
	{
		Output = 0;
		BoundsSize = 0.0f;
		LayerResolution = 64;
		LayerTilesCount = 8;
		InnerRadius = 50.0f;
		OuterRadius = 125.0f;
		FactorK = 0.5f;
		SDFTexture = nullptr;
	}
};

class VOLUMETRICFXRENDERING_API FVolumetricFXSDFComputeShaderInterface
{
public:
	// Executes this shader on the render thread
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVolumetircFXSDFCSParams Params, TFunction<void(int OutputVal)> AsyncCallback);
	
	// Executes this shader on the render thread from the game thread via EnqueueRenderThreadCommand
	static void DispatchGameThread(FVolumetircFXSDFCSParams Params, TFunction<void(int OutputVal)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			DispatchRenderThread(RHICmdList, Params, AsyncCallback);
		});
	}
	
	// Dispatches this shader. Can be called from any thread
	static void Dispatch(FVolumetircFXSDFCSParams Params, TFunction<void(int OutputVal)> AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, AsyncCallback);
		}
	}
	
	static UTextureRenderTarget2D* BuildSDFRenderTarget(UObject* Outer, const uint32& LayerBaseSize);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const int, Value);

UCLASS()
class VOLUMETRICFXRENDERING_API UFVolumetricFXSDFComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	virtual void Activate() override
	{
		FVolumetircFXSDFCSParams Params;
		Params.BoundsSize = BoundsSize;
		Params.SDFTexture = SDFRenderTarget;
		
		FVolumetricFXSDFComputeShaderInterface::Dispatch(Params, [this](int OutputVal)
		{
			if (Completed.IsBound())
			{
				Completed.Broadcast(OutputVal);
			}
		});
	}
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UFVolumetricFXSDFComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(
		UObject* WorldContextObject,
		int32 VoxelCount,
		TArray<FVector> VoxelPointLocation,
		FVector BoundsOrigin,
		float BoundsSize,
		UTextureRenderTarget2D* SDFRenderTarget)
	{
		UFVolumetricFXSDFComputeShaderLibrary_AsyncExecution* Action = NewObject<UFVolumetricFXSDFComputeShaderLibrary_AsyncExecution>();
		
		Action->SDFRenderTarget = SDFRenderTarget;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}
	
	UPROPERTY(BlueprintAssignable)
	FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted Completed;
	
	uint32 VoxelCount;
	TArray<FVector3f> VoxelPointLocation;
	float BoundsSize;
	
	TObjectPtr<UTextureRenderTarget2D> SDFRenderTarget;
};