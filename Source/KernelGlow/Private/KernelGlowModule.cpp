#include "KernelGlow.h"
#include "KgViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogKernelGlow);

namespace
{
	// 验证管线自截图：kg.AutoScreenshot 1 后延迟 3 秒执行 HighResShot；kg.AutoQuit 1 则截图后 1 秒退出进程
	TAutoConsoleVariable<int32> CVarKgAutoScreenshot(
		TEXT("kg.AutoScreenshot"), 0,
		TEXT("KernelGlow: 延迟 3 秒自动执行 HighResShot（验证管线用）"),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarKgAutoQuit(
		TEXT("kg.AutoQuit"), 0,
		TEXT("KernelGlow: 自截图完成后 1 秒自动退出进程"),
		ECVF_Default);

	const double KgAutoScreenshotDelaySeconds = 3.0;
	const double KgAutoQuitDelaySeconds = 1.0;
}

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

	// 自截图 ticker（验证管线）
	AutoScreenshotStartTime = FPlatformTime::Seconds();
	AutoScreenshotDone = false;
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FKernelGlowModule::AutoScreenshotTick), 0.0f);
}

bool FKernelGlowModule::AutoScreenshotTick(float DeltaTime)
{
	if (QuitDeadline > 0.0 && FPlatformTime::Seconds() >= QuitDeadline)
	{
		if (GEngine)
		{
			GEngine->Exec(nullptr, TEXT("quit"), *GLog);
		}
		return false;
	}

	if (AutoScreenshotDone || CVarKgAutoScreenshot.GetValueOnGameThread() == 0)
	{
		return true; // 继续 tick，等待开启
	}

	if (!GEngine || !GEngine->GameViewport)
	{
		return true;
	}

	if (FPlatformTime::Seconds() - AutoScreenshotStartTime < KgAutoScreenshotDelaySeconds)
	{
		return true;
	}

	UE_LOG(LogKernelGlow, Log, TEXT("KernelGlow: auto screenshot fired (t=%.1fs)"), FPlatformTime::Seconds() - AutoScreenshotStartTime);
	GEngine->GameViewport->Exec(nullptr, TEXT("HighResShot 1920x1080"), *GLog);
	AutoScreenshotDone = true;

	if (CVarKgAutoQuit.GetValueOnGameThread() != 0)
	{
		QuitDeadline = FPlatformTime::Seconds() + KgAutoQuitDelaySeconds;
		return true; // 继续 tick 等待退出时刻
	}

	return false; // 完成即停
}

void FKernelGlowModule::ShutdownModule()
{
	ViewExtension.Reset();
}

IMPLEMENT_MODULE(FKernelGlowModule, KernelGlow)

