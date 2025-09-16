/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "MySimpleComputeShader.generated.h"

USTRUCT()
struct VOLUMETRICFXRENDERING_API FVolumetircFXSDFCSParams
{
	GENERATED_BODY()
	
	int32 Output;
	
	// Input
	int32 VoxelCount;
	TArray<FVector3f> VoxelPointLocation;
	FVector BoundsOrigin;
	float BoundsSize;
	
	// Output
	TObjectPtr<UTextureRenderTarget2D> SDFTexture;
	
	FVolumetircFXSDFCSParams()
	{
		Output = 0;
		
		VoxelCount = 0;
		BoundsOrigin = FVector::ZeroVector;
		BoundsSize = 0.0f;
		
		SDFTexture = nullptr;
	}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class VOLUMETRICFXRENDERING_API FMySimpleComputeShaderInterface
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
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted, const int, Value);

UCLASS()
class VOLUMETRICFXRENDERING_API UMySimpleComputeShaderLibrary_AsyncExecution : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	virtual void Activate() override
	{
		FVolumetircFXSDFCSParams Params;
		//Params.VoxelPointLocation = VoxelPointLocation;
		Params.BoundsOrigin = BoundsOrigin;
		Params.BoundsSize = BoundsSize;
		Params.VoxelCount = VoxelCount;
		
		Params.SDFTexture = SDFRenderTarget;
		
		FMySimpleComputeShaderInterface::Dispatch(Params, [this](int OutputVal)
		{
			if (Completed.IsBound())
			{
				Completed.Broadcast(OutputVal);
			}
		});
	}
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Category = "ComputeShader", WorldContext = "WorldContextObject"))
	static UMySimpleComputeShaderLibrary_AsyncExecution* ExecuteBaseComputeShader(
		UObject* WorldContextObject,
		int32 VoxelCount,
		TArray<FVector> VoxelPointLocation,
		FVector BoundsOrigin,
		float BoundsSize,
		UTextureRenderTarget2D* SDFRenderTarget)
	{
		UMySimpleComputeShaderLibrary_AsyncExecution* Action = NewObject<UMySimpleComputeShaderLibrary_AsyncExecution>();
		
		Action->SDFRenderTarget = SDFRenderTarget;
		Action->RegisterWithGameInstance(WorldContextObject);
		return Action;
	}
	
	UPROPERTY(BlueprintAssignable)
	FOnMySimpleComputeShaderLibrary_AsyncExecutionCompleted Completed;
	
	int32 VoxelCount;
	TArray<FVector> VoxelPointLocation;
	FVector BoundsOrigin;
	float BoundsSize;
	
	TObjectPtr<UTextureRenderTarget2D> SDFRenderTarget;
};