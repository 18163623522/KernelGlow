#include "KernelGlow.h"
#include "KgViewExtension.h"
#include "KernelGlowSettings.h"
#include "KgKernelMath.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/BulkData.h"
#include "Math/Float16Color.h"
#include "Misc/EngineVersionComparison.h"

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
	if (KernelTextureInternal)
	{
		KernelTextureInternal->RemoveFromRoot();
		KernelTextureInternal = nullptr;
	}
	ViewExtension.Reset();
}

UTexture2D* FKernelGlowModule::GetOrCreateKernelTexture()
{
	if (KernelTextureInternal && IsValid(KernelTextureInternal))
	{
		return KernelTextureInternal;
	}

	const UKernelGlowSettings* Settings = GetDefault<UKernelGlowSettings>();

	// 用户 kernel 贴图优先
	if (!Settings->KernelTexturePath.IsNull())
	{
		if (UTexture2D* Loaded = LoadObject<UTexture2D>(nullptr, *Settings->KernelTexturePath.ToString()))
		{
			KernelTextureInternal = Loaded;
			Loaded->AddToRoot();
			UE_LOG(LogKernelGlow, Log, TEXT("KernelGlow: kernel texture from %s"), *Settings->KernelTexturePath.ToString());
			return Loaded;
		}
		UE_LOG(LogKernelGlow, Warning, TEXT("KernelGlow: kernel texture %s failed to load, using procedural fallback."), *Settings->KernelTexturePath.ToString());
	}

	// 程序化回退：C++ 生成形状 + CPU mip 链
	constexpr int32 KernelSize = 256;
	TArray<FLinearColor> Mip0;
	KgKernelMath::GenerateProceduralKernel(Settings->ProceduralKernel, KernelSize, Mip0);
	TArray<TArray<FLinearColor>> CpuMips;
	KgKernelMath::ComputeCpuMips(Mip0, KernelSize, CpuMips);

	UTexture2D* Tex = UTexture2D::CreateTransient(KernelSize, KernelSize, PF_FloatRGBA, TEXT("KgKernelTex"));
	if (!Tex)
	{
		UE_LOG(LogKernelGlow, Error, TEXT("KernelGlow: failed to create kernel texture."));
		return nullptr;
	}
	Tex->NeverStream = true;
	Tex->MipGenSettings = TMGS_NoMipmaps;

	FTexturePlatformData* PlatformData = Tex->GetPlatformData();
	PlatformData->Mips.Reset();
	auto FillMip = [&](int32 MipSize, const TArray<FLinearColor>& Pixels)
	{
		FTexture2DMipMap* Mip = new FTexture2DMipMap(MipSize, MipSize, 1);
		PlatformData->Mips.Add(Mip);
		const int64 MipBytes = (int64)MipSize * (int64)MipSize * (int64)sizeof(FFloat16Color);
		Mip->BulkData.Lock(LOCK_READ_WRITE);
		void* Data = Mip->BulkData.Realloc(MipBytes);
		FFloat16Color* Dst = (FFloat16Color*)Data;
		for (int32 I = 0; I < MipSize * MipSize; ++I)
		{
			Dst[I] = FFloat16Color(Pixels[I]);
		}
		Mip->BulkData.Unlock();
	};

	FillMip(KernelSize, Mip0);
	for (int32 M = 0; M < CpuMips.Num(); ++M)
	{
		FillMip(FMath::Max(KernelSize >> (M + 1), 1), CpuMips[M]);
	}
	Tex->UpdateResource();

	KernelTextureInternal = Tex;
	Tex->AddToRoot();
	UE_LOG(LogKernelGlow, Log, TEXT("KernelGlow: procedural kernel created (shape=%d, size=%d, mips=%d)"),
		Settings->ProceduralKernel, KernelSize, PlatformData->Mips.Num());
	return Tex;
}

IMPLEMENT_MODULE(FKernelGlowModule, KernelGlow)

