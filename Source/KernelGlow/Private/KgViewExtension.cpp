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
	for (int32 S = 0; S < 3; ++S)
	{
		Cached.ScaleRadius[S] = Settings->Scales[S].Radius;
		Cached.ScaleIntensity[S] = Settings->Scales[S].Intensity;
		Cached.ScalesEnabled[S] = Settings->Scales[S].bEnabled;
		Cached.ScaleTintR[S] = Settings->Scales[S].Tint.R;
		Cached.ScaleTintG[S] = Settings->Scales[S].Tint.G;
		Cached.ScaleTintB[S] = Settings->Scales[S].Tint.B;
	}
	Cached.CoreRadius = Settings->Core.Radius;
	Cached.CoreStrength = Settings->Core.Strength;
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

	TArray<FRDGTextureRef> Levels;
	KgRender::RenderGlowPipeline(GraphBuilder, SceneColor, Params, Levels);

	constexpr int32 TapCount = 8;
	if (Levels.Num() == 0 || KernelTextureRHI == nullptr)
	{
		return;
	}

	const float BaseWidth = (float)SceneColor->Desc.Extent.X;
	const float Overall = Cached.OverallIntensity * Cached.Gain;

	// 三尺度卷积（鸣潮式：近距青白 / 中距蓝 / Wide 深蓝低能量）
	const int32 DebugSource = CVarKgDebugSource.GetValueOnRenderThread();
	for (int32 S = 0; S < 3; ++S)
	{
		if (!Cached.ScalesEnabled[S] || Cached.ScaleIntensity[S] <= 0.0f)
		{
			continue;
		}
		const int32 Level = KgKernelMath::ChooseLevelForRadius(Cached.ScaleRadius[S], BaseWidth, TapCount, Levels.Num() - 1);
		const float ExtentPx = KgKernelMath::ComputeExtentPx(Cached.ScaleRadius[S], BaseWidth, Level);

		FRDGTextureRef RdgKernel = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(KernelTextureRHI, TEXT("KgKernelTex")));

		FRDGTextureRef Convolved;
		KgRender::RenderConvolve(GraphBuilder, Levels[Level], RdgKernel, ExtentPx, TapCount,
			FLinearColor(Cached.ScaleTintR[S], Cached.ScaleTintG[S], Cached.ScaleTintB[S], 1.0f), Convolved);

		const float AddScale = Overall * Cached.ScaleIntensity[S];
		KgRender::RenderDebugAdd(GraphBuilder, SceneColor, Convolved, AddScale,
			CVarKgDebugReplace.GetValueOnRenderThread() != 0);
	}

	// 核心回填：Levels[0] 小核卷积（半径 3px → span 6px，4×4 tap）
	if (Cached.CoreStrength > 0.0f && Cached.CoreRadius > 0.5f)
	{
		FRDGTextureRef RdgKernel = GraphBuilder.RegisterExternalTexture(
			CreateRenderTarget(KernelTextureRHI, TEXT("KgKernelTex")));
		FRDGTextureRef Core;
		KgRender::RenderConvolve(GraphBuilder, Levels[0], RdgKernel, Cached.CoreRadius * 2.0f, 4,
			FLinearColor(1, 1, 1, 1), Core);
		KgRender::RenderDebugAdd(GraphBuilder, SceneColor, Core, Overall * Cached.CoreStrength,
			CVarKgDebugReplace.GetValueOnRenderThread() != 0);
	}

}
