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

	/** kernel 贴图（用户贴图优先，否则程序化圆盘/六边形/星芒），带 CPU mip 链。仅游戏线程调用 */
	UTexture2D* GetOrCreateKernelTexture();

	/** 渲染线程只读访问（不得创建） */
	UTexture2D* GetKernelTextureForRender() const { return KernelTextureInternal; }

private:
	bool AutoScreenshotTick(float DeltaTime);

	TSharedPtr<class FKgViewExtension, ESPMode::ThreadSafe> ViewExtension;
	UTexture2D* KernelTextureInternal = nullptr; // AddToRoot 防 GC
	double AutoScreenshotStartTime = 0.0;
	double QuitDeadline = 0.0;
	bool AutoScreenshotDone = false;
};
