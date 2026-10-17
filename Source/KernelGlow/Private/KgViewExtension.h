#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

class FViewInfo;

/**
 * KernelGlow 渲染注入。
 * 钩子：PrePostProcessPass_RenderThread（RealBloom 5.7 同款，每帧稳定触发）；
 * SceneColor 取自 FSceneTextureUniformParameters，全部 pass 以 UAV 原地写入。
 */
class FKgViewExtension final : public FSceneViewExtensionBase
{
public:
	/** 设置 POD 快照：游戏线程每帧写入，渲染线程只读（禁止 RT 访问设置 UObject） */
	struct FCachedSettings
	{
		bool bEnabled = false;
		bool bCompositeHDR = true;
		float Threshold = 1.0f;
		float OverallIntensity = 1.0f;
		float Gain = 1.0f;
		float Scale0Radius = 0.06f;
		float Scale0Intensity = 0.8f;
		int32 Quality = 1;
	};

	explicit FKgViewExtension(const FAutoRegister& AutoRegister);

	//~ ISceneViewExtension
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

private:
	static bool ShouldProcessView_RenderThread(const FSceneView& View);

	// 游戏线程（BeginRenderViewFamily）缓存，渲染线程只读：禁止 RT 访问 UObject
	FTextureRHIRef KernelTextureRHI;
	int32 KernelTexSizeX = 0;
	FCachedSettings Cached;
};
