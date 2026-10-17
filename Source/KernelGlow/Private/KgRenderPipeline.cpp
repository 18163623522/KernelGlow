#include "KgRenderPipeline.h"
#include "KgKernelMath.h"
#include "KgShaders.h"
#include "KernelGlow.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "ScreenPass.h"

IMPLEMENT_GLOBAL_SHADER(FKgPlaceholderCS, "/Plugin/KernelGlow/Private/KgPlaceholder.usf", "KgPlaceholderMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgBrightPassCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgBrightPassMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgDownsampleCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgDownsampleMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgDebugAddCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgDebugAddMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgConvolveCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgConvolveMainCS", SF_Compute);

namespace
{
	using KgSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>;

	FRDGTextureRef CreateLevelTexture(FRDGBuilder& GraphBuilder, FIntPoint Size, const TCHAR* Name)
	{
		FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(
			Size, PF_FloatRGBA, FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV));
		return GraphBuilder.CreateTexture(Desc, Name);
	}

	FIntPoint Halve(FIntPoint Size)
	{
		return FIntPoint(
			FMath::Max(FMath::DivideAndRoundUp(Size.X, 2), 1),
			FMath::Max(FMath::DivideAndRoundUp(Size.Y, 2), 1));
	}
}

namespace KgRender
{
	void RenderGlowPipeline(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, const FGlowParams& Params, TArray<FRDGTextureRef>& OutLevels)
	{
		OutLevels.Reset();
		if (!SceneColor)
		{
			return;
		}

		const FIntPoint FullSize = SceneColor->Desc.Extent;
		const FIntPoint Level0Size = Halve(FullSize);

		// Bright pass：软阈值提取到半分辨率
		FRDGTextureRef Level0 = CreateLevelTexture(GraphBuilder, Level0Size, TEXT("KgBright0"));
		{
			FKgBrightPassCS::FParameters* Params0 = GraphBuilder.AllocParameters<FKgBrightPassCS::FParameters>();
			Params0->OutputSizeX = Level0Size.X;
			Params0->OutputSizeY = Level0Size.Y;
			Params0->Threshold = Params.Threshold;
			Params0->Knee = Params.Knee;
			Params0->InputTexture = SceneColor;
			Params0->InputSampler = KgSampler::GetRHI();
			Params0->OutputTexture = GraphBuilder.CreateUAV(Level0);

			TShaderMapRef<FKgBrightPassCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgBrightPass"), Shader, Params0,
				FComputeShaderUtils::GetGroupCount(Level0Size, FIntPoint(8, 8)));
		}

		// 金字塔降采样
		FRDGTextureRef Prev = Level0;
		FIntPoint PrevSize = Level0Size;
		for (int32 Level = 1; Level < Params.PyramidLevels; ++Level)
		{
			const FIntPoint NextSize = Halve(PrevSize);
			FRDGTextureRef Next = CreateLevelTexture(GraphBuilder, NextSize, *FString::Printf(TEXT("KgDown%d"), Level));

			FKgDownsampleCS::FParameters* ParamsD = GraphBuilder.AllocParameters<FKgDownsampleCS::FParameters>();
			ParamsD->DownOutputSizeX = NextSize.X;
			ParamsD->DownOutputSizeY = NextSize.Y;
			ParamsD->DownTexelSizeX = 1.0f / PrevSize.X;
			ParamsD->DownTexelSizeY = 1.0f / PrevSize.Y;
			ParamsD->DownInputTexture = Prev;
			ParamsD->DownInputSampler = KgSampler::GetRHI();
			ParamsD->DownOutputTexture = GraphBuilder.CreateUAV(Next);

			TShaderMapRef<FKgDownsampleCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgDownsample"), Shader, ParamsD,
				FComputeShaderUtils::GetGroupCount(NextSize, FIntPoint(8, 8)));

			Prev = Next;
			PrevSize = NextSize;
			OutLevels.Add(Next);
		}

		OutLevels.Add(Level0);
	}

	void RenderPlaceholderPass(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor)
	{
		if (!SceneColor)
		{
			return;
		}

		FKgPlaceholderCS::FParameters* Params = GraphBuilder.AllocParameters<FKgPlaceholderCS::FParameters>();
		Params->SceneColorUAV = GraphBuilder.CreateUAV(SceneColor);

		const FIntPoint TextureSize = SceneColor->Desc.Extent;
		const FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(TextureSize, FIntPoint(8, 8));

		TShaderMapRef<FKgPlaceholderCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgPlaceholder"), Shader, Params, GroupCount);
	}

	void RenderDebugAdd(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureRef BrightTexture, float Scale, bool bReplace)
	{
		if (!SceneColor || !BrightTexture)
		{
			return;
		}

		const FIntPoint FullSize = SceneColor->Desc.Extent;
		FKgDebugAddCS::FParameters* Params = GraphBuilder.AllocParameters<FKgDebugAddCS::FParameters>();
		Params->AddSizeX = BrightTexture->Desc.Extent.X;
		Params->AddSizeY = BrightTexture->Desc.Extent.Y;
		Params->AddScale = Scale;
		Params->AddReplace = bReplace ? 1.0f : 0.0f;
		Params->AddTexture = BrightTexture;
		Params->AddSampler = KgSampler::GetRHI();
		Params->SceneColorUAV = GraphBuilder.CreateUAV(SceneColor);

		TShaderMapRef<FKgDebugAddCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgDebugAdd"), Shader, Params,
			FComputeShaderUtils::GetGroupCount(FullSize, FIntPoint(8, 8)));
	}
}

	namespace KgRender
	{
		void RenderConvolve(FRDGBuilder& GraphBuilder, FRDGTextureRef LevelTexture, FRDGTextureRef KernelTexture, float ExtentPx, int32 TapCount, FRDGTextureRef& OutConvolved)
		{
			OutConvolved = nullptr;
			if (!LevelTexture || !KernelTexture)
			{
				return;
			}

			const FIntPoint LevelSize = LevelTexture->Desc.Extent;
			FRDGTextureRef Out = CreateLevelTexture(GraphBuilder, LevelSize, TEXT("KgConvolved"));

			FKgConvolveCS::FParameters* Params = GraphBuilder.AllocParameters<FKgConvolveCS::FParameters>();
			Params->ConvOutSizeX = LevelSize.X;
			Params->ConvOutSizeY = LevelSize.Y;
			Params->ConvTexelSizeX = 1.0f / LevelSize.X;
			Params->ConvTexelSizeY = 1.0f / LevelSize.Y;
			Params->ConvExtentPx = ExtentPx;
			Params->ConvKernelLod = KgKernelMath::ComputeKernelLod(KernelTexture->Desc.Extent.X, TapCount);
			Params->ConvTapCount = TapCount;
			Params->ConvInputTexture = LevelTexture;
			Params->ConvInputSampler = KgSampler::GetRHI();
			Params->ConvKernelTexture = KernelTexture;
			Params->ConvKernelSampler = KgSampler::GetRHI();
			Params->ConvOutputTexture = GraphBuilder.CreateUAV(Out);

			TShaderMapRef<FKgConvolveCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgConvolve"), Shader, Params,
				FComputeShaderUtils::GetGroupCount(LevelSize, FIntPoint(8, 8)));

			OutConvolved = Out;
		}
	}
