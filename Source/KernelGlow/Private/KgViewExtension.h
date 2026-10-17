#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

class FViewInfo;

/**
 * KernelGlow 渲染注入。
 * 签名对齐 UE 5.7（参照 RealBloom 在 5.6/5.7 验证过的接口）。
 */
class FKgViewExtension final : public FSceneViewExtensionBase
{
public:
	explicit FKgViewExtension(const FAutoRegister& AutoRegister);

	//~ ISceneViewExtension
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

private:
	bool ShouldProcessView_RenderThread(const FSceneView& View) const;
};
