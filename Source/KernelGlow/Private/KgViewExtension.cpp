#include "KgViewExtension.h"
#include "KernelGlow.h"
#include "KernelGlowSettings.h"

FKgViewExtension::FKgViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

/**
 * 视口过滤策略（设计文档 4 节）：
 * - SceneCapture 视口 fail-closed 跳过（防递归）
 * - 材质/Niagara 等资产预览视口跳过
 * - 编辑器主视口与 PIE/游戏视口应用
 */
bool FKgViewExtension::ShouldProcessView_RenderThread(const FSceneView& View) const
{
	if (View.bIsSceneCapture || View.bIsReflectionCapture || View.bIsPlanarReflection)
	{
		return false;
	}
	return true;
}

void FKgViewExtension::PrePostProcessPass_RenderThread(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FPostProcessingInputs& Inputs)
{
	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	if (!ShouldProcessView_RenderThread(View))
	{
		return;
	}

	// Task 3 起在此接入 RDG pass 链（placeholder tint → 完整卷积管线）
}
