#include "KernelGlow.h"
#include "KgViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(LogKernelGlow);

void FKernelGlowModule::StartupModule()
{
	// 注册 shader 目录映射：/Plugin/KernelGlow -> <插件>/Shaders
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("KernelGlow")))
	{
		const FString ShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/KernelGlow"), ShaderDir);
		UE_LOG(LogKernelGlow, Log, TEXT("KernelGlow: shader directory mapped: %s"), *ShaderDir);
	}

	// 引擎初始化完成后注册 ViewExtension（与 RealBloom 相同的时机，保证后处理通道可用）
	FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		if (!ViewExtension.IsValid())
		{
			ViewExtension = FSceneViewExtensions::NewExtension<FKgViewExtension>();
			UE_LOG(LogKernelGlow, Log, TEXT("KernelGlow: ViewExtension registered."));
		}
	});
}

void FKernelGlowModule::ShutdownModule()
{
	ViewExtension.Reset();
}

IMPLEMENT_MODULE(FKernelGlowModule, KernelGlow)
