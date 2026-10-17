#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/** Task 3 placeholder（验证用，保留作参考） */
class FKgPlaceholderCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKgPlaceholderCS);
	SHADER_USE_PARAMETER_STRUCT(FKgPlaceholderCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorUAV)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("ENABLE_TYPED_UAV_LOAD"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
};

/** Task 4 亮度提取（软阈值 + 半分辨率降采样） */
class FKgBrightPassCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKgBrightPassCS);
	SHADER_USE_PARAMETER_STRUCT(FKgBrightPassCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, OutputSizeX)
		SHADER_PARAMETER(float, OutputSizeY)
		SHADER_PARAMETER(float, Threshold)
		SHADER_PARAMETER(float, Knee)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};

/** Task 4 金字塔 13-tap 降采样 */
class FKgDownsampleCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKgDownsampleCS);
	SHADER_USE_PARAMETER_STRUCT(FKgDownsampleCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, DownOutputSizeX)
		SHADER_PARAMETER(float, DownOutputSizeY)
		SHADER_PARAMETER(float, DownTexelSizeX)
		SHADER_PARAMETER(float, DownTexelSizeY)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DownInputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, DownInputSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, DownOutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};

/** Task 4 调试叠加（scene += bright * Scale） */
class FKgDebugAddCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKgDebugAddCS);
	SHADER_USE_PARAMETER_STRUCT(FKgDebugAddCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, AddSizeX)
		SHADER_PARAMETER(float, AddSizeY)
		SHADER_PARAMETER(float, AddScale)
		SHADER_PARAMETER(float, AddReplace)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, AddTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, AddSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorUAV)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("ENABLE_TYPED_UAV_LOAD"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
};

/** Task 6 单尺度卷积 */
class FKgConvolveCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FKgConvolveCS);
	SHADER_USE_PARAMETER_STRUCT(FKgConvolveCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(float, ConvOutSizeX)
		SHADER_PARAMETER(float, ConvOutSizeY)
		SHADER_PARAMETER(float, ConvTexelSizeX)
		SHADER_PARAMETER(float, ConvTexelSizeY)
		SHADER_PARAMETER(float, ConvExtentPx)
		SHADER_PARAMETER(float, ConvKernelLod)
		SHADER_PARAMETER(int, ConvTapCount)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ConvInputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, ConvInputSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ConvKernelTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, ConvKernelSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, ConvOutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};
