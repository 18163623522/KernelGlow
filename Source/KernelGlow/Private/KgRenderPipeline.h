#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FRDGTexture;

namespace KgRender
{
	struct FGlowParams
	{
		float Threshold = 1.0f;
		float Knee = 0.5f;
		int32 PyramidLevels = 5;   // 质量档映射：0=3 1=5 2=6
		bool bUseStencilMask = false;
		bool bUseLuminanceSource = true;
		int32 StencilValues[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
		int32 NumStencilValues = 0;
		class FRDGTextureSRV* StencilTextureSRV = nullptr; // Texture2D<uint2> SRV
	};

	/**
	 * 亮度提取 + 金字塔降采样。
	 * 输出 BrightLevel0（半分辨率 bright pass），供 Task 6 卷积 / Task 4 调试叠加使用。
	 */
		struct FSourceInputs
	{
		FRDGTextureSRVRef StencilTextureSRV;
	};

	void RenderGlowPipeline(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, const FGlowParams& Params, const FSourceInputs& SourceInputs, TArray<FRDGTextureRef>& OutLevels);

	/** 在指定层级上做单尺度卷积（含染色），返回同尺寸卷积结果 */
	void RenderConvolve(FRDGBuilder& GraphBuilder, FRDGTextureRef LevelTexture, FRDGTextureRef KernelTexture, float ExtentPx, int32 TapCount, const FLinearColor& Tint, FRDGTextureRef& OutConvolved);

	/** Task 3 placeholder：原地常量绿写入（回归验证用） */
	void RenderPlaceholderPass(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor);

	/** stencil 可视化（诊断）：直接把 stencil 值写到 scene */
	void RenderStencilDebug(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureSRVRef StencilSRV);

	/** 合成：scene += tex * Scale（双线性上采样，Task 6 起为正式合成路径） */
	void RenderDebugAdd(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureRef BrightTexture, float Scale, bool bReplace = false);
}
