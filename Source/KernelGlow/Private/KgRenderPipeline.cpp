#include "KgRenderPipeline.h"
#include "KgShaders.h"
#include "KernelGlow.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "ScreenPass.h"

IMPLEMENT_GLOBAL_SHADER(FKgPlaceholderCS, "/Plugin/KernelGlow/Private/KgPlaceholder.usf", "KgPlaceholderMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgBrightPassCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgBrightPassMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgDownsampleCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgDownsampleMainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FKgDebugAddCS, "/Plugin/KernelGlow/Private/KgBrightPass.usf", "KgDebugAddMainCS", SF_Compute);

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
	void RenderGlowPipeline(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, const FGlowParams& Params, FRDGTextureRef& OutBrightLevel0)
	{
		OutBrightLevel0 = nullptr;
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
		}

		OutBrightLevel0 = Level0;
	}

	void RenderDebugAdd(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureRef BrightTexture, float Scale)
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
		Params->AddTexture = BrightTexture;
		Params->AddSampler = KgSampler::GetRHI();
		Params->SceneColorUAV = GraphBuilder.CreateUAV(SceneColor);

		TShaderMapRef<FKgDebugAddCS> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("KgDebugAdd"), Shader, Params,
			FComputeShaderUtils::GetGroupCount(FullSize, FIntPoint(8, 8)));
	}
}
