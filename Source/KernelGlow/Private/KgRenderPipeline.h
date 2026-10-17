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
	void RenderGlowPipeline(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, const FGlowParams& Params, FRDGTextureRef& OutBrightLevel0);

	/** Task 4 临时：scene += bright0 * Scale（双线性上采样） */
	void RenderDebugAdd(FRDGBuilder& GraphBuilder, FRDGTextureRef SceneColor, FRDGTextureRef BrightTexture, float Scale);
}
