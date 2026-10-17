#include "KgViewExtension.h"
#include "KernelGlow.h"
#include "KernelGlowSettings.h"
#include "KgRenderPipeline.h"
#include "KgKernelMath.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Modules/ModuleManager.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

// A/B 验证用运行时旁路开关（文件作用域，保证外部链接）
TAutoConsoleVariable<int32> CVarKgForceDisable(
	TEXT("kg.ForceDisable"), 0,
	TEXT("KernelGlow: 运行时强制旁路注入（A/B 验证用）"),
	ECVF_Default);

extern TAutoConsoleVariable<int32> CVarKgDebugReplace;
extern TAutoConsoleVariable<int32> CVarKgDebugSource;

FKgViewExtension::FKgViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

/** 游戏线程：缓存设置快照与 kernel RHI 引用，渲染线程不触碰 UObject */
void FKgViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();
	Cached.bEnabled = Settings->bEnabled;
	Cached.bCompositeHDR = Settings->bCompositeHDR;
	Cached.Threshold = Settings->Threshold;
	Cached.OverallIntensity = Settings->OverallIntensity;
	Cached.Gain = Settings->Gain;
	Cached.Scale0Radius = Settings->Scales[0].Radius;
	Cached.Scale0Intensity = Settings->Scales[0].Intensity;
	Cached.Quality = Settings->Quality;

	UTexture2D* KernelTex = FModuleManager::GetModuleChecked<FKernelGlowModule>(TEXT("KernelGlow")).GetKernelTextureForRender();
	if (KernelTex && KernelTex->GetResource())
	{
		KernelTextureRHI = KernelTex->GetResource()->TextureRHI;
		KernelTexSizeX = KernelTex->GetSizeX();
	}
	else
	{
		KernelTextureRHI.SafeRelease();
		KernelTexSizeX = 0;
	}
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

/**
 * 每帧稳定触发的注入点（RealBloom 5.7 同款）：
 * SceneColor 取自 FSceneTextureUniformParameters（SceneTexturesConfig.h:42），
 * 全部 pass 以 UAV 原地写入，不走返回值链。
 */
void FKgViewExtension::PrePostProcessPass_RenderThread(
	FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs)
{
	if (!Cached.bEnabled || !Inputs.SceneTextures)
	{
		return;
	}
	if (!ShouldProcessView_RenderThread(View))
	{
		return;
	}

	FRDGTextureRef SceneColor = (*Inputs.SceneTextures)->SceneColorTexture;
	if (!SceneColor)
	{
		return;
	}

	// 诊断 4：常量绿写入（机制验证）
	if (CVarKgDebugSource.GetValueOnRenderThread() == 4)
	{
		KgRender::RenderPlaceholderPass(GraphBuilder, SceneColor);
		return;
	}

	KgRender::FGlowParams Params;
	Params.Threshold = Cached.Threshold;
	Params.Knee = 0.5f;
	Params.PyramidLevels = (Cached.Quality == 0) ? 3 : (Cached.Quality == 2 ? 6 : 5);

	// 诊断 5：完整管线 + 常量绿（管线是否破坏渲染图）
	if (CVarKgDebugSource.GetValueOnRenderThread() == 5)
	{
		TArray<FRDGTextureRef> Levels5;
		KgRender::RenderGlowPipeline(GraphBuilder, SceneColor, Params, Levels5);
		KgRender::RenderPlaceholderPass(GraphBuilder, SceneColor);
		return;
	}

	// 诊断 1：bright0 叠加（跳过卷积）
	if (CVarKgDebugSource.GetValueOnRenderThread() == 1)
	{
		TArray<FRDGTextureRef> Levels1;
		KgRender::RenderGlowPipeline(GraphBuilder, SceneColor, Params, Levels1);
		if (Levels1.Num() > 0)
		{
			KgRender::RenderDebugAdd(GraphBuilder, SceneColor, Levels1[0], 1.0f, false);
		}
		return;
	}
	Params.Threshold = Cached.Threshold;
	Params.Knee = 0.5f;
	Params.PyramidLevels = (Cached.Quality == 0) ? 3 : (Cached.Quality == 2 ? 6 : 5);

	TArray<FRDGTextureRef> Levels;
	KgRender::RenderGlowPipeline(GraphBuilder, SceneColor, Params, Levels);

	constexpr int32 TapCount = 8;
	if (Levels.Num() == 0 || KernelTextureRHI == nullptr)
	{
		return;
	}

	const float BaseWidth = (float)SceneColor->Desc.Extent.X;
	const int32 Level = KgKernelMath::ChooseLevelForRadius(Cached.Scale0Radius, BaseWidth, TapCount, Levels.Num() - 1);
	const float ExtentPx = KgKernelMath::ComputeExtentPx(Cached.Scale0Radius, BaseWidth, Level);

	FRDGTextureRef RdgKernel = GraphBuilder.RegisterExternalTexture(
		CreateRenderTarget(KernelTextureRHI, TEXT("KgKernelTex")));

	FRDGTextureRef Convolved;
	KgRender::RenderConvolve(GraphBuilder, Levels[Level], RdgKernel, ExtentPx, TapCount, Convolved);

	const float AddScale = Cached.OverallIntensity * Cached.Scale0Intensity * Cached.Gain;
	const int32 DebugSource = CVarKgDebugSource.GetValueOnRenderThread();
	FRDGTextureRef AddTexture = (DebugSource == 1) ? Levels[0]
		: (DebugSource == 2) ? Levels[Levels.Num() - 1]
		: (DebugSource == 3) ? RdgKernel
		: Convolved;
	const float AddScaleFinal = (DebugSource != 0) ? 1.0f : AddScale;
	KgRender::RenderDebugAdd(GraphBuilder, SceneColor, AddTexture, AddScaleFinal,
		CVarKgDebugReplace.GetValueOnRenderThread() != 0);

	static std::atomic<int32> ConvDiag{ 0 };
	if (ConvDiag.fetch_add(1) < 2 || ConvDiag.load() % 120 == 0)
	{
		UE_LOG(LogKernelGlow, Log, TEXT("PrePP run #%d levels=%d level=%d extent=%.1f scale=%.2f"),
			ConvDiag.load(), Levels.Num(), Level, ExtentPx, AddScaleFinal);
	}
}
