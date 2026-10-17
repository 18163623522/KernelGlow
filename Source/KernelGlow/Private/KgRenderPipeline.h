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
		float DebugAddScale = 2.0f; // Task 4 临时：scene += bright0 * Scale
	};

	/**
	 * 亮度提取 + 金字塔降采样。
	 * 输出 BrightLevel0（半分辨率 bright pass），供 Task 6 卷积 / Task 4 调试叠加使用。
	 */
		void RenderGlowPipeline(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, const FGlowParams& Params, TArray<FRDGTextureRef>& OutLevels);

	/** 在指定层级上做单尺度卷积（含染色），返回同尺寸卷积结果 */
	void RenderConvolve(FRDGBuilder& GraphBuilder, FRDGTextureRef LevelTexture, FRDGTextureRef KernelTexture, float ExtentPx, int32 TapCount, const FLinearColor& Tint, FRDGTextureRef& OutConvolved);

	/** Task 3 placeholder：原地常量绿写入（回归验证用） */
	void RenderPlaceholderPass(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor);

	/** 合成：scene += tex * Scale（双线性上采样，Task 6 起为正式合成路径） */
	void RenderDebugAdd(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureRef BrightTexture, float Scale, bool bReplace = false);
}
