#include "KgRenderPipeline.h"
#include "KgShaders.h"
#include "KernelGlow.h"
#include "RenderGraphUtils.h"

IMPLEMENT_GLOBAL_SHADER(FKgPlaceholderCS, "/Plugin/KernelGlow/Private/KgPlaceholder.usf", "KgPlaceholderMainCS", SF_Compute);

namespace KgRender
{
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

		TShaderMapRef<FKgPlaceholderCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("KernelGlow Placeholder"),
			ComputeShader,
			Params,
			GroupCount);
	}
}
