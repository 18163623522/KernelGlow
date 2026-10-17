#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"

class FViewInfo;

/**
 * KernelGlow 渲染注入。
 * UE 5.7 签名（PostProcessInputs 改为 uniform buffer，改用 SubscribeToPostProcessingPass 链式通道）。
 * 槽位选择（设计文档 5/9 节）：
 * - bCompositeHDR=true：MotionBlur 槽（BL_SceneColorBeforeBloom）= HDR、tonemap 前、曝光计量之后（无呼吸）
 * - bCompositeHDR=false：AfterTonemap 槽 = 风格化 LDR 合成
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
	virtual void SubscribeToPostProcessingPass(ISceneViewExtension::EPostProcessingPass Pass, const FSceneView& InView, FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

private:
	FScreenPassTexture PostProcessPassAfterBloomLocation_RenderThread(
		FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(
		FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

	static bool ShouldProcessView_RenderThread(const FSceneView& View);
};
