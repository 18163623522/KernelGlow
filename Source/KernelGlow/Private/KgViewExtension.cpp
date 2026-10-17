#include "KgViewExtension.h"
#include "KernelGlow.h"
#include "KernelGlowSettings.h"
#include "KgRenderPipeline.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "SceneView.h"

extern TAutoConsoleVariable<int32> CVarKgForceDisable;

// A/B 验证用运行时旁路开关（文件作用域，保证外部链接）
TAutoConsoleVariable<int32> CVarKgForceDisable(
	TEXT("kg.ForceDisable"), 0,
	TEXT("KernelGlow: 运行时强制旁路注入（A/B 验证用）"),
	ECVF_RenderThreadSafe);

FKgViewExtension::FKgViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

/**
 * 视口过滤策略（设计文档 4 节）：
 * SceneCapture / 反射 / 平面反射视口 fail-closed 跳过（防递归）；
 * 其余（编辑器主视口、PIE、游戏视口）应用。资产预览视口过滤在 Task 4 补充。
 */
bool FKgViewExtension::ShouldProcessView_RenderThread(const FSceneView& View)
{
	if (CVarKgForceDisable.GetValueOnRenderThread() != 0)
	{
		return false;
	}
	if (View.bIsSceneCapture || View.bIsReflectionCapture || View.bIsPlanarReflection)
	{
		return false;
	}
	return true;
}

void FKgViewExtension::SubscribeToPostProcessingPass(
	ISceneViewExtension::EPostProcessingPass Pass,
	const FSceneView& InView,
	FPostProcessingPassDelegateArray& InOutPassCallbacks,
	bool bIsPassEnabled)
{
	static std::atomic<int32> DiagCount{ 0 };
	if (DiagCount.fetch_add(1) < 8)
	{
		UE_LOG(LogKernelGlow, Log, TEXT("Subscribe pass=%d enabled=%d HDR=%d"),
			static_cast<int32>(Pass), bIsPassEnabled ? 1 : 0,
			GetDefault<UKernelGlowSettings>()->bCompositeHDR ? 1 : 0);
	}

	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();
	if (!Settings || !Settings->bEnabled || !bIsPassEnabled)
	{
		return;
	}

	if (Settings->bCompositeHDR && Pass == ISceneViewExtension::EPostProcessingPass::AfterDOF)
	{
		// AfterDOF 槽位：始终订阅（引擎对 DOF 前四个槽不裁剪）；HDR、tonemap 前
		InOutPassCallbacks.Add(FPostProcessingPassDelegate::CreateRaw(
			this, &FKgViewExtension::PostProcessPassAfterBloomLocation_RenderThread));
	}
	else if (!Settings->bCompositeHDR && Pass == ISceneViewExtension::EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FPostProcessingPassDelegate::CreateRaw(
			this, &FKgViewExtension::PostProcessPassAfterTonemap_RenderThread));
	}
}

FScreenPassTexture FKgViewExtension::PostProcessPassAfterBloomLocation_RenderThread(
	FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	static std::atomic<int32> RunCount{ 0 };
	if (RunCount.fetch_add(1) < 3)
	{
		UE_LOG(LogKernelGlow, Log, TEXT("AfterDOF callback ran"));
	}
	FScreenPassTexture SceneColor = Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	if (!SceneColor.IsValid() || !ShouldProcessView_RenderThread(View))
	{
		return SceneColor;
	}

	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();
	KgRender::FGlowParams Params;
	Params.Threshold = Settings->Threshold;
	Params.Knee = 0.5f;
	Params.PyramidLevels = (Settings->Quality == 0) ? 3 : (Settings->Quality == 2 ? 6 : 5);

	FRDGTextureRef Bright0 = nullptr;
	KgRender::RenderGlowPipeline(GraphBuilder, SceneColor.Texture, Params, Bright0);

	// Task 4 临时合成：scene += bright0 * 2（Task 6 换正式合成）
	KgRender::RenderDebugAdd(GraphBuilder, SceneColor.Texture, Bright0, Params.DebugAddScale);
	return SceneColor;
}

FScreenPassTexture FKgViewExtension::PostProcessPassAfterTonemap_RenderThread(
	FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
	FScreenPassTexture SceneColor = Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	if (!SceneColor.IsValid() || !ShouldProcessView_RenderThread(View))
	{
		return SceneColor;
	}

	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();
	KgRender::FGlowParams Params;
	Params.Threshold = Settings->Threshold;
	Params.PyramidLevels = (Settings->Quality == 0) ? 3 : (Settings->Quality == 2 ? 6 : 5);

	FRDGTextureRef Bright0 = nullptr;
	KgRender::RenderGlowPipeline(GraphBuilder, SceneColor.Texture, Params, Bright0);
	KgRender::RenderDebugAdd(GraphBuilder, SceneColor.Texture, Bright0, Params.DebugAddScale);
	return SceneColor;
}