#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKernelGlow, Log, All);

class FKernelGlowModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool AutoScreenshotTick(float DeltaTime);

	TSharedPtr<class FKgViewExtension, ESPMode::ThreadSafe> ViewExtension;
	double AutoScreenshotStartTime = 0.0;
	double QuitDeadline = 0.0;
	bool AutoScreenshotDone = false;
};
