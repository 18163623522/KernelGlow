#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKernelGlow, Log, All);

class UTexture2D;

class FKernelGlowModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** kernel 贴图（用户贴图优先，否则程序化圆盘/六边形/星芒），带 CPU mip 链；懒创建 + AddToRoot 防 GC */
	UTexture2D* GetOrCreateKernelTexture();

private:
	bool AutoScreenshotTick(float DeltaTime);

	TSharedPtr<class FKgViewExtension, ESPMode::ThreadSafe> ViewExtension;
	UTexture2D* KernelTextureInternal = nullptr; // AddToRoot 防 GC
	double AutoScreenshotStartTime = 0.0;
	double QuitDeadline = 0.0;
	bool AutoScreenshotDone = false;
};
