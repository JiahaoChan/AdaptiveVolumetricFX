/**
 * Plugin AdaptiveVolumetricFX
 *		Create interactive Volumetric Clouds, Volumetric Fog and other FX.
 * Copyright Technical Artist - Jiahao.Chan, Individual. All Rights Reserved.
 */

#include "MySimpleComputeShader.h"

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

//#include "VolumetricFXRendering/Public/MySimpleComputeShader.h"

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

DECLARE_STATS_GROUP(TEXT("MySimpleComputeShader"), STATGROUP_MySimpleComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("MySimpleComputeShader Execute"), STAT_MySimpleComputeShader_Execute, STATGROUP_MySimpleComputeShader);

class VOLUMETRICFXRENDERING_API FMySimpleComputeShader : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMySimpleComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FMySimpleComputeShader, FGlobalShader);
	
	class FMySimpleComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain
	<
		FMySimpleComputeShader_Perm_TEST
	>;
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		/*
		* Here's where you define one or more of the input parameters for your shader.
		* Some examples:
		*/
		// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
		// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;
		
		// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
		// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();
		
		// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];
		
		// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
		// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;
		
		// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
		// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;
		
		// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)
		
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<int>, Input)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<int>, Output)
		
		SHADER_PARAMETER(uint32, VoxelCount)
		SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<FVector3f>, VoxelPointLocation)
		SHADER_PARAMETER(FVector3f, BoundsOrigin)
		SHADER_PARAMETER(float, BoundsSize)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<FVector4f>, SDFTexture)
		
	END_SHADER_PARAMETER_STRUCT()
	
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		return true;
	}
    
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		OutEnvironment.SetDefine(TEXT("NUMTHREADSX"), 8);
		OutEnvironment.SetDefine(TEXT("NUMTHREADSY"), 8);
		OutEnvironment.SetDefine(TEXT("NUMTHREADSZ"), 1);
		
		if (PermutationVector.Get< FMySimpleComputeShader_Perm_TEST >() != 0)
		{
			OutEnvironment.SetDefine(TEXT("TEST"), TEXT("1"));
		}
		
		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMySimpleComputeShader, "/Plugin/VolumetricFXRenderingShaders/MySimpleComputeShader.usf", "Main", SF_Compute);

void FMySimpleComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVolumetircFXSDFCSParams Params, TFunction<void(int OutputVal)> AsyncCallback)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_MySimpleComputeShader_Execute);
		DECLARE_GPU_STAT(MySimpleComputeShader);
		RDG_EVENT_SCOPE(GraphBuilder, "MySimpleComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, MySimpleComputeShader);
		
		typename FMySimpleComputeShader::FPermutationDomain PermutationVector;
		
		// Add any static permutation options here
		//PermutationVector.Set<FMySimpleComputeShader::FMySimpleComputeShader_Perm_TEST>(1);
		
		TShaderMapRef<FMySimpleComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		
		if (ComputeShader.IsValid())
		{
			FMySimpleComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FMySimpleComputeShader::FParameters>();
			
			const void* RawData = (void*)Params.Input.GetData();
			int NumInputs = Params.Input.Num();
			int InputSize = sizeof(int32);
			
			FRDGBufferRef InputBuffer = CreateUploadBuffer(GraphBuilder, TEXT("InputBuffer"), InputSize, NumInputs, RawData, InputSize * NumInputs);
			PassParameters->Input = GraphBuilder.CreateSRV(FRDGBufferSRVDesc(InputBuffer, PF_R32_SINT));
			
			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(int32), 1), TEXT("OutputBuffer"));
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_SINT));
			
			PassParameters->BoundsOrigin = FVector3f(0.0f, 0.0f, 0.0f);
			PassParameters->BoundsSize = 100.0f;
			
			FTextureResource* TextureResource = Params.SDFTexture->GetResource();
			check(TextureResource);
			FRDGTextureRef OutputTextureRDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(TextureResource->GetTexture2DRHI(), TEXT("SDFTexture")));
			PassParameters->SDFTexture = GraphBuilder.CreateUAV(OutputTextureRDG);
			
			FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(8, 8, 1), FComputeShaderUtils::kGolden2DGroupSize);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteMySimpleComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});
			
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
							AsyncCallback(OutVal);
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
		}
		else
		{
			// We silently exit here as we don't want to crash the game if the shader is not found or has an error.
		}
	}
	GraphBuilder.Execute();
}