/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "VolumetricFXSDFComputeShader.h"

#include "CoreMinimal.h"
#include "VolumetricFXRendering.h"
#include "MeshPassProcessor.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "MeshMaterialShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "ShaderCompilerCore.h"
//#include "Renderer/Private/ScenePrivate.h"
#include "EngineDefines.h"
#include "RendererInterface.h"
#include "RenderResource.h"
#include "RenderGraphResources.h"

#include "RenderGraphResources.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MaterialShader.h"
#include "RHIGPUReadback.h"

DECLARE_STATS_GROUP(TEXT("FVolumetricFXSDFComputeShader"), STATGROUP_VolumetricFXSDFComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("FVolumetricFXSDFComputeShader Execute"), STAT_VolumetricFXSDFComputeShader_Execute, STATGROUP_VolumetricFXSDFComputeShader);

class VOLUMETRICFXRENDERING_API FVolumetricFXSDFComputeShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVolumetricFXSDFComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVolumetricFXSDFComputeShader, FGlobalShader);
	
	class FSDFBlendFunctionMethod : SHADER_PERMUTATION_INT("SDF_BLEND_FUNCTION", 7);
	using FPermutationDomain = TShaderPermutationDomain<
		FSDFBlendFunctionMethod
	>;
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)
		
		SHADER_PARAMETER(uint32, VoxelCount)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector3f>, VoxelPointLocations)
		SHADER_PARAMETER(float, BoundsSize)
		SHADER_PARAMETER(uint32, LayerResolution)
		SHADER_PARAMETER(uint32, LayerTilesCount)
		SHADER_PARAMETER(float, InnerRadius)
		SHADER_PARAMETER(float, OuterRadius)
		SHADER_PARAMETER(float, FactorK)
		SHADER_PARAMETER(uint32, ConeCount)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FSubductionConeShape>, ConeShapes)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float>, SDFTexture)
	END_SHADER_PARAMETER_STRUCT()
	
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		bool bShouldCompile = true;
		switch (PermutationVector.Get<FSDFBlendFunctionMethod>())
		{
			// Only use Root Function for the moment.
			case 1:
				bShouldCompile = true;
				break;
			default:
				bShouldCompile = false;
				break;
		}
		
		return bShouldCompile;
	}
    
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		OutEnvironment.SetDefine(TEXT("THREADTILES"), 8);
		
		OutEnvironment.SetDefine(TEXT("SDF_BLEND_FUNCTION"), PermutationVector.Get<FSDFBlendFunctionMethod>());
		
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_EXPONENTIAL"), 0);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_ROOT"), 1);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_SIGMOID"), 2);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_QUADRATICPOLYNOMIAL"), 3);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_CUBICPOLYNOMIAL"), 4);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_QUARTICPOLYNOMIAL"), 5);
		OutEnvironment.SetDefine(TEXT("SMIN_FUNCTION_CIRCULAR"), 6);
		
		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
};

IMPLEMENT_GLOBAL_SHADER(FVolumetricFXSDFComputeShader, "/Plugin/VolumetricFXRenderingShaders/VolumetricFXSDFComputeShader.usf", "Main", SF_Compute);

void FVolumetricFXSDFComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVolumetircFXSDFCSParams Params, TFunction<void(int OutputVal)> AsyncCallback)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_VolumetricFXSDFComputeShader_Execute);
		DECLARE_GPU_STAT(VolumetricFXSDFComputeShader);
		RDG_EVENT_SCOPE(GraphBuilder, "VolumetricFXSDFComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VolumetricFXSDFComputeShader);
		
		typename FVolumetricFXSDFComputeShader::FPermutationDomain PermutationVector;
		
		// Default to be 1 - SMin Root for the moment.
		PermutationVector.Set<FVolumetricFXSDFComputeShader::FSDFBlendFunctionMethod>(1);
		
		TShaderMapRef<FVolumetricFXSDFComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		
		if (ComputeShader.IsValid() && Params.VoxelPointLocations.Num() > 0)
		{
			FVolumetricFXSDFComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FVolumetricFXSDFComputeShader::FParameters>();
			/*
			const void* RawData = (void*)Params.Input.GetData();
			int NumInputs = Params.Input.Num();
			int InputSize = sizeof(int32);
			
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32_SINT));
			*/
			
			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1), TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));
			
			{
                const void* RawData = (void*)Params.VoxelPointLocations.GetData();
                uint32 NumInputs = Params.VoxelPointLocations.Num();
                uint32 InputSize = sizeof(FVector3f);
                FRDGBufferRef VoxelPointLocationsBuffer = CreateUploadBuffer(GraphBuilder, TEXT("VoxelPointLocationsBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
                PassParameters->VoxelPointLocations = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(VoxelPointLocationsBuffer, PF_R32G32B32F));
                PassParameters->VoxelCount = NumInputs;
			}
			
			PassParameters->BoundsSize = Params.BoundsSize;
			PassParameters->LayerResolution = Params.LayerResolution;
			PassParameters->LayerTilesCount = Params.LayerTilesCount;
			PassParameters->InnerRadius = Params.InnerRadius;
			PassParameters->OuterRadius = Params.OuterRadius;
			PassParameters->FactorK = Params.FactorK;
			
			{
				uint32 NumInputs = Params.ConeShapes.Num();
				FRDGBufferRef ConeShapesBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("ConeShapesBuffer"), Params.ConeShapes);
				PassParameters->ConeShapes = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(ConeShapesBuffer));
				PassParameters->ConeCount = NumInputs;
			}
			
			FTextureResource* TextureResource = Params.SDFTexture->GetResource();
			check(TextureResource);
			FRDGTextureRef SDFTextureRDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(TextureResource->GetTexture2DRHI(), TEXT("VolumetricFXSDFTexture")));
			PassParameters->SDFTexture = GraphBuilder.CreateUAV(SDFTextureRDG, ERDGUnorderedAccessViewFlags::None, PF_R16F);
			
			FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(8, 8, 1), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteVolumetricFXSDFComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});
			
			/*
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteMySimpleComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, 0u);
			
			auto RunnerFunc = [GPUBufferReadback, AsyncCallback](auto&& RunnerFunc)->void
			{
				if (GPUBufferReadback->IsReady())
				{
					int32* Buffer = (int32*)GPUBufferReadback->Lock(1);
					int OutVal = Buffer[0];
					
					GPUBufferReadback->Unlock();
					AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]()
						{
							if (AsyncCallback.IsSet())
							{
								AsyncCallback(OutVal);
							}
						});
					delete GPUBufferReadback;
				}
				else
				{
					AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()->void
						{
							RunnerFunc(RunnerFunc);
						});
				}
			};
			
			AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]()->void
				{
					RunnerFunc(RunnerFunc);
				});
			*/
		}
		else
		{
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.
		}
	}
	GraphBuilder.Execute();
}

UTextureRenderTarget2D* FVolumetricFXSDFComputeShaderInterface::BuildSDFRenderTarget(UObject* Outer, const uint32& LayerBaseSize)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(IsValid(Outer) ? Outer : GetTransientPackage());
	RT->RenderTargetFormat = RTF_R16f;
	RT->SRGB = false;
	RT->bCanCreateUAV = true;
	RT->AddressX = TA_Clamp;
	RT->AddressY = TA_Clamp;
	float Resolution = LayerBaseSize * LayerBaseSize * LayerBaseSize;
	RT->InitAutoFormat(Resolution, Resolution);
	return RT;
}