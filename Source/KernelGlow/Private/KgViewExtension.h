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
		float ScaleRadius[3] = { 0.06f, 0.16f, 0.42f };
		float ScaleIntensity[3] = { 0.8f, 0.35f, 0.0f };
		bool ScalesEnabled[3] = { true, true, false };
		float ScaleTintR[3] = { 0.62f, 0.44f, 0.29f };
		float ScaleTintG[3] = { 0.85f, 0.71f, 0.49f };
		float ScaleTintB[3] = { 1.0f, 1.0f, 1.0f };
		float CoreRadius = 3.0f;
		float CoreStrength = 0.6f;
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
